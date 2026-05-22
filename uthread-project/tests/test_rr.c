#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uthread.h"

/* ── shared event log ────────────────────────────────────────────────────── */

static char log_buf[512];
static int  log_pos = 0;

static void log_event(const char *ev)
{
    log_pos += snprintf(log_buf + log_pos,
                        sizeof(log_buf) - log_pos, "%s ", ev);
}

/* ── thread functions ────────────────────────────────────────────────────── */

/* Each thread yields N times, logging its name at every step. */
static void *rr_thread_2step(void *arg)
{
    const char *name = (const char *)arg;
    char step[8];

    snprintf(step, sizeof(step), "%s1", name);  log_event(step);
    uthread_yield();
    snprintf(step, sizeof(step), "%s2", name);  log_event(step);

    return NULL;
}

static void *rr_thread_3step(void *arg)
{
    const char *name = (const char *)arg;
    char step[8];

    snprintf(step, sizeof(step), "%s1", name);  log_event(step);
    uthread_yield();
    snprintf(step, sizeof(step), "%s2", name);  log_event(step);
    uthread_yield();
    snprintf(step, sizeof(step), "%s3", name);  log_event(step);

    return NULL;
}

/* ── test: three threads interleave one step at a time ───────────────────── */

static void test_rr_three_threads(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_RR);
    uthread_create(NULL, rr_thread_2step, "T1", 0);
    uthread_create(NULL, rr_thread_2step, "T2", 0);
    uthread_create(NULL, rr_thread_2step, "T3", 0);
    uthread_start();

    /*
     * RR: each yield re-enqueues at the tail.
     * Queue starts: [T1, T2, T3]
     * T1→T11, yield → [T2, T3, T1]
     * T2→T21, yield → [T3, T1, T2]
     * T3→T31, yield → [T1, T2, T3]
     * T1→T12, exit  → [T2, T3]
     * T2→T22, exit  → [T3]
     * T3→T32, exit  → []
     * Expected: T11 T21 T31 T12 T22 T32
     */
    assert(strcmp(log_buf, "T11 T21 T31 T12 T22 T32 ") == 0);
    printf("  [PASS] RR three threads 2-step: %s\n", log_buf);
}

/* ── test: unequal step counts still interleave fairly ───────────────────── */

static void test_rr_unequal_steps(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_RR);
    uthread_create(NULL, rr_thread_3step, "A", 0);
    uthread_create(NULL, rr_thread_2step, "B", 0);
    uthread_start();

    /*
     * A has 3 steps, B has 2 steps.
     * Queue: [A, B]
     * A→A1, yield → [B, A]
     * B→B1, yield → [A, B]
     * A→A2, yield → [B, A]
     * B→B2, exit  → [A]
     * A→A3, exit  → []
     * Expected: A1 B1 A2 B2 A3
     */
    assert(strcmp(log_buf, "A1 B1 A2 B2 A3 ") == 0);
    printf("  [PASS] RR unequal steps: %s\n", log_buf);
}

/* ── test: single thread under RR ───────────────────────────────────────── */

static void test_rr_single(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_RR);
    uthread_create(NULL, rr_thread_2step, "X", 0);
    uthread_start();

    /* Only one thread: yield puts it at tail but it's still the only one. */
    assert(strcmp(log_buf, "X1 X2 ") == 0);
    printf("  [PASS] RR single thread: %s\n", log_buf);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_rr] RR scheduling tests\n");

    test_rr_three_threads();
    test_rr_unequal_steps();
    test_rr_single();

    printf("[test_rr] all tests passed.\n");
    return 0;
}
