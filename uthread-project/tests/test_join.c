#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uthread.h"

/* join / delete / list 测试 */

static char log_buf[512];
static int  log_pos = 0;

static void log_event(const char *ev)
{
    log_pos += snprintf(log_buf + log_pos,
                        sizeof(log_buf) - log_pos, "%s ", ev);
}

/* 测试 1：join 阻塞调用者直到目标退出 */

static int worker_tid_g;

static void *worker(void *arg) { (void)arg; log_event("W"); return NULL; }

static void *joiner(void *arg)
{
    (void)arg;
    uthread_join(worker_tid_g, NULL);
    log_event("J");
    return NULL;
}

static void test_join_blocks(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL,          joiner, NULL, 0);
    uthread_create(&worker_tid_g, worker, NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "W J ") == 0);
    printf("  [PASS] join 阻塞调用者: %s\n", log_buf);
}

/* 测试 2：join 已退出的线程立即返回 */

static int worker2_tid_g;

static void *worker2(void *arg) { (void)arg; log_event("W2"); return NULL; }

static void *joiner2(void *arg)
{
    (void)arg;
    assert(uthread_join(worker2_tid_g, NULL) == 0);
    log_event("J2_imm");
    return NULL;
}

static void test_join_already_exited(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(&worker2_tid_g, worker2, NULL, 0);
    uthread_create(NULL,           joiner2, NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "W2 J2_imm ") == 0);
    printf("  [PASS] join 已退出线程立即返回: %s\n", log_buf);
}

/* 测试 3：retval 通过 join 正确传递 */

static int retval_worker_tid_g;

static void *retval_worker(void *arg)
{
    (void)arg;
    uthread_exit((void *)42);
    return NULL;
}

static void *retval_joiner(void *arg)
{
    (void)arg;
    void *rv = NULL;
    uthread_join(retval_worker_tid_g, &rv);
    assert((long)rv == 42);
    log_event("RV_OK");
    return NULL;
}

static void test_join_retval(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL,                 retval_joiner, NULL, 0);
    uthread_create(&retval_worker_tid_g, retval_worker, NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "RV_OK ") == 0);
    printf("  [PASS] retval 经 join 正确传递\n");
}

/* 测试 4：删除 READY 线程后该线程不会运行 */

static void *should_not_run(void *arg)
{
    (void)arg;
    log_event("DELETED_THREAD_RAN");
    return NULL;
}

static void test_delete_ready(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    int del_tid;
    uthread_create(&del_tid, should_not_run, NULL, 0);
    uthread_create(NULL,     worker,         NULL, 0);

    assert(uthread_delete(del_tid) == 0);
    uthread_start();

    assert(strstr(log_buf, "DELETED_THREAD_RAN") == NULL);
    assert(strstr(log_buf, "W") != NULL);
    printf("  [PASS] 已删除的 READY 线程不会被调度\n");
}

/* 测试 5：删除非法或 RUNNING 线程返回 -1 */

static void test_delete_invalid(void)
{
    uthread_init(UTHREAD_SCHED_FIFO);
    assert(uthread_delete(9999) == -1);
    assert(uthread_delete(all_threads->tid) == -1); /* 主线程正在运行 */
    printf("  [PASS] 删除非法或 RUNNING 线程返回 -1\n");
}

/* 测试 6：uthread_list 输出不崩溃 */

static void test_list(void)
{
    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, worker, NULL, 1);
    uthread_create(NULL, worker, NULL, 2);

    printf("  --- uthread_list 输出 ---\n");
    uthread_list();
    printf("  -------------------------\n");
    printf("  [PASS] uthread_list 正常输出\n");
}

/* 测试 7：同一目标的第二个 joiner 被拒绝 */

static int double_join_target_tid;

static void *double_join_target(void *arg) { (void)arg; return NULL; }

static void *first_joiner(void *arg)
{
    (void)arg;
    assert(uthread_join(double_join_target_tid, NULL) == 0);
    log_event("J1_ok");
    return NULL;
}

static void *second_joiner(void *arg)
{
    (void)arg;
    assert(uthread_join(double_join_target_tid, NULL) == -1);
    log_event("J2_rejected");
    return NULL;
}

static void test_double_join_rejected(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL,                    first_joiner,       NULL, 0);
    uthread_create(NULL,                    second_joiner,      NULL, 0);
    uthread_create(&double_join_target_tid, double_join_target, NULL, 0);
    uthread_start();

    assert(strstr(log_buf, "J2_rejected") != NULL);
    assert(strstr(log_buf, "J1_ok")       != NULL);
    printf("  [PASS] 重复 join 同一目标返回 -1\n");
}

/* 测试 8：删除有 join 等待者的目标返回 -1 */

static int delete_with_waiter_target_tid;

static void *long_worker(void *arg) { (void)arg; uthread_yield(); return NULL; }

static void *waiter_thread(void *arg)
{
    (void)arg;
    uthread_join(delete_with_waiter_target_tid, NULL);
    log_event("WAITER_DONE");
    return NULL;
}

static void *deleter_thread(void *arg)
{
    (void)arg;
    assert(uthread_delete(delete_with_waiter_target_tid) == -1);
    log_event("DEL_REJECTED");
    return NULL;
}

static void test_delete_with_waiter_rejected(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL,                           waiter_thread,  NULL, 0);
    uthread_create(NULL,                           deleter_thread, NULL, 0);
    uthread_create(&delete_with_waiter_target_tid, long_worker,    NULL, 0);
    uthread_start();

    assert(strstr(log_buf, "DEL_REJECTED") != NULL);
    assert(strstr(log_buf, "WAITER_DONE")  != NULL);
    printf("  [PASS] 删除有等待者的目标返回 -1\n");
}

int main(void)
{
    printf("[test_join] join / delete / list 测试\n");

    test_join_blocks();
    test_join_already_exited();
    test_join_retval();
    test_delete_ready();
    test_delete_invalid();
    test_list();
    test_double_join_rejected();
    test_delete_with_waiter_rejected();

    printf("[test_join] 全部通过\n");
    return 0;
}
