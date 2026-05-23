#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uthread.h"

/* FIFO 调度测试 */

static char log_buf[512];
static int  log_pos = 0;

static void log_event(const char *ev)
{
    log_pos += snprintf(log_buf + log_pos,
                        sizeof(log_buf) - log_pos, "%s ", ev);
}

static void *fifo_thread(void *arg)
{
    log_event((const char *)arg);
    return NULL;
}

static void *yield_thread(void *arg)
{
    const char *name = (const char *)arg;
    char step[8];
    snprintf(step, sizeof(step), "%s1", name); log_event(step);
    uthread_yield();
    snprintf(step, sizeof(step), "%s2", name); log_event(step);
    return NULL;
}

static void test_fifo_order(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, fifo_thread, "T1", 0);
    uthread_create(NULL, fifo_thread, "T2", 0);
    uthread_create(NULL, fifo_thread, "T3", 0);
    uthread_start();

    /* 无 yield：严格按创建顺序执行 */
    assert(strcmp(log_buf, "T1 T2 T3 ") == 0);
    printf("  [PASS] FIFO 顺序（无 yield）: %s\n", log_buf);
}

static void test_fifo_yield_order(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, yield_thread, "A", 0);
    uthread_create(NULL, yield_thread, "B", 0);
    uthread_create(NULL, yield_thread, "C", 0);
    uthread_start();

    /* yield 后入队尾，三轮交替 */
    assert(strcmp(log_buf, "A1 B1 C1 A2 B2 C2 ") == 0);
    printf("  [PASS] FIFO 配合 yield: %s\n", log_buf);
}

int main(void)
{
    printf("[test_fifo] FIFO 调度测试\n");

    test_fifo_order();
    test_fifo_yield_order();

    printf("[test_fifo] 全部通过\n");
    return 0;
}
