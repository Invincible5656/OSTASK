#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uthread.h"

/* 优先级调度测试 */

static char log_buf[512];
static int  log_pos = 0;

static void log_event(const char *ev)
{
    log_pos += snprintf(log_buf + log_pos,
                        sizeof(log_buf) - log_pos, "%s ", ev);
}

static void *prio_thread(void *arg)
{
    log_event((const char *)arg);
    return NULL;
}

static void test_basic_priority(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_PRIORITY);
    /* 故意倒序创建，验证调度器按优先级而非创建顺序 */
    uthread_create(NULL, prio_thread, "LOW",  5);
    uthread_create(NULL, prio_thread, "MID",  3);
    uthread_create(NULL, prio_thread, "HIGH", 1);
    uthread_start();

    assert(strcmp(log_buf, "HIGH MID LOW ") == 0);
    printf("  [PASS] 基本优先级顺序: %s\n", log_buf);
}

static void test_equal_priority(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_PRIORITY);
    uthread_create(NULL, prio_thread, "T1", 2);
    uthread_create(NULL, prio_thread, "T2", 2);
    uthread_create(NULL, prio_thread, "T3", 2);
    uthread_start();

    /* 相同优先级回退到 FIFO 顺序 */
    assert(strcmp(log_buf, "T1 T2 T3 ") == 0);
    printf("  [PASS] 相同优先级回退 FIFO: %s\n", log_buf);
}

/* 验证 set_priority 在下次调度时立即生效 */
static int t2_tid_g, t3_tid_g;

static void *set_prio_t1(void *arg)
{
    (void)arg;
    log_event("T1a");
    uthread_set_priority(t3_tid_g, 1); /* 把 T3 提到比自己更高 */
    uthread_yield();
    log_event("T1b");
    return NULL;
}

static void *set_prio_t2(void *arg) { (void)arg; log_event("T2"); return NULL; }
static void *set_prio_t3(void *arg) { (void)arg; log_event("T3"); return NULL; }

static void test_set_priority_runtime(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_PRIORITY);
    uthread_create(NULL,         set_prio_t1, NULL, 2);
    uthread_create(&t2_tid_g,    set_prio_t2, NULL, 5);
    uthread_create(&t3_tid_g,    set_prio_t3, NULL, 8);
    uthread_start();

    assert(strcmp(log_buf, "T1a T3 T1b T2 ") == 0);
    printf("  [PASS] 运行时 set_priority 立即生效: %s\n", log_buf);
}

static void test_set_priority_invalid(void)
{
    uthread_init(UTHREAD_SCHED_PRIORITY);
    assert(uthread_set_priority(9999, 1) == -1);
    printf("  [PASS] 无效 tid 返回 -1\n");
}

int main(void)
{
    printf("[test_priority] 优先级调度测试\n");

    test_basic_priority();
    test_equal_priority();
    test_set_priority_runtime();
    test_set_priority_invalid();

    printf("[test_priority] 全部通过\n");
    return 0;
}
