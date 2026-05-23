#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uthread.h"

/* 基础测试：yield、exit、上下文切换 */

static char log_buf[256];
static int  log_pos = 0;

static void log_event(const char *ev)
{
    log_pos += snprintf(log_buf + log_pos,
                        sizeof(log_buf) - log_pos, "%s ", ev);
}

static void *thread_a(void *arg)
{
    (void)arg;
    log_event("A1"); uthread_yield(); log_event("A2");
    return NULL;
}

static void *thread_b(void *arg)
{
    (void)arg;
    log_event("B1"); uthread_yield(); log_event("B2");
    return NULL;
}

static void *thread_single(void *arg)
{
    (void)arg;
    log_event("S");
    return NULL;
}

static void test_yield_order(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, thread_a, NULL, 0);
    uthread_create(NULL, thread_b, NULL, 0);
    uthread_start();

    /* A1→yield→B1→yield→A2→exit→B2→exit */
    assert(strcmp(log_buf, "A1 B1 A2 B2 ") == 0);
    printf("  [PASS] yield 交替执行: %s\n", log_buf);
}

static void test_exit_no_yield(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, thread_single, NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "S ") == 0);
    printf("  [PASS] 单线程退出: %s\n", log_buf);
}

static void test_start_empty(void)
{
    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_start();
    printf("  [PASS] 空队列启动正常返回\n");
}

int main(void)
{
    printf("[test_basic] yield / exit / 上下文切换\n");

    test_yield_order();
    test_exit_no_yield();
    test_start_empty();

    printf("[test_basic] 全部通过\n");
    return 0;
}
