#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uthread.h"

/* 时间片轮转 RR 调度测试 */

static char log_buf[512];
static int  log_pos = 0;

static void log_event(const char *ev)
{
    log_pos += snprintf(log_buf + log_pos,
                        sizeof(log_buf) - log_pos, "%s ", ev);
}

static void *rr_thread_2step(void *arg)
{
    const char *name = (const char *)arg;
    char step[8];
    snprintf(step, sizeof(step), "%s1", name); log_event(step);
    uthread_yield();
    snprintf(step, sizeof(step), "%s2", name); log_event(step);
    return NULL;
}

static void *rr_thread_3step(void *arg)
{
    const char *name = (const char *)arg;
    char step[8];
    snprintf(step, sizeof(step), "%s1", name); log_event(step);
    uthread_yield();
    snprintf(step, sizeof(step), "%s2", name); log_event(step);
    uthread_yield();
    snprintf(step, sizeof(step), "%s3", name); log_event(step);
    return NULL;
}

static void test_rr_three_threads(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_RR);
    uthread_create(NULL, rr_thread_2step, "T1", 0);
    uthread_create(NULL, rr_thread_2step, "T2", 0);
    uthread_create(NULL, rr_thread_2step, "T3", 0);
    uthread_start();

    assert(strcmp(log_buf, "T11 T21 T31 T12 T22 T32 ") == 0);
    printf("  [PASS] RR 三线程双步轮转: %s\n", log_buf);
}

static void test_rr_unequal_steps(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_RR);
    uthread_create(NULL, rr_thread_3step, "A", 0);
    uthread_create(NULL, rr_thread_2step, "B", 0);
    uthread_start();

    assert(strcmp(log_buf, "A1 B1 A2 B2 A3 ") == 0);
    printf("  [PASS] RR 不等步数公平轮转: %s\n", log_buf);
}

static void test_rr_single(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_RR);
    uthread_create(NULL, rr_thread_2step, "X", 0);
    uthread_start();

    assert(strcmp(log_buf, "X1 X2 ") == 0);
    printf("  [PASS] RR 单线程: %s\n", log_buf);
}

int main(void)
{
    printf("[test_rr] RR 调度测试\n");

    test_rr_three_threads();
    test_rr_unequal_steps();
    test_rr_single();

    printf("[test_rr] 全部通过\n");
    return 0;
}
