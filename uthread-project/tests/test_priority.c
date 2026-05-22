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

/* ── test 1: threads run in priority order (lower number = higher priority) */

static void *prio_thread(void *arg)
{
    log_event((const char *)arg);
    return NULL;
}

static void test_basic_priority(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_PRIORITY);
    /* Create in reverse priority order to prove scheduler reorders them. */
    uthread_create(NULL, prio_thread, "LOW",  5);
    uthread_create(NULL, prio_thread, "MID",  3);
    uthread_create(NULL, prio_thread, "HIGH", 1);
    uthread_start();

    /* Expected: HIGH(1) → MID(3) → LOW(5) */
    assert(strcmp(log_buf, "HIGH MID LOW ") == 0);
    printf("  [PASS] basic priority order: %s\n", log_buf);
}

/* ── test 2: equal-priority threads run in FIFO (creation) order ─────────── */

static void test_equal_priority(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_PRIORITY);
    uthread_create(NULL, prio_thread, "T1", 2);
    uthread_create(NULL, prio_thread, "T2", 2);
    uthread_create(NULL, prio_thread, "T3", 2);
    uthread_start();

    /* All same priority → FIFO order preserved. */
    assert(strcmp(log_buf, "T1 T2 T3 ") == 0);
    printf("  [PASS] equal priority falls back to FIFO: %s\n", log_buf);
}

/* ── test 3: uthread_set_priority takes effect on the next dispatch ──────── */

/*
 * Setup: T1(pri=2), T2(pri=5), T3(pri=8)
 * T1 dispatched first (lowest priority number).
 * T1 logs "T1a", promotes T3 to priority=1 (higher than T1), then yields.
 * Next dispatch picks T3 (now pri=1, lowest in queue).
 * T3 logs "T3", exits.
 * Then T1 resumes (pri=2), logs "T1b", exits.
 * Finally T2 (pri=5) runs.
 * Expected: T1a T3 T1b T2
 */
static int t2_tid_g, t3_tid_g;

static void *set_prio_t1(void *arg)
{
    (void)arg;
    log_event("T1a");
    uthread_set_priority(t3_tid_g, 1); /* promote T3 above T1 */
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
    uthread_create(NULL, set_prio_t1, NULL, 2);
    uthread_create(&t2_tid_g, set_prio_t2, NULL, 5);
    uthread_create(&t3_tid_g, set_prio_t3, NULL, 8);
    uthread_start();

    assert(strcmp(log_buf, "T1a T3 T1b T2 ") == 0);
    printf("  [PASS] set_priority at runtime: %s\n", log_buf);
}

/* ── test 4: set_priority returns -1 for unknown tid ────────────────────── */

static void test_set_priority_invalid(void)
{
    uthread_init(UTHREAD_SCHED_PRIORITY);
    assert(uthread_set_priority(9999, 1) == -1);
    printf("  [PASS] set_priority on unknown tid returns -1\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_priority] Priority scheduling tests\n");

    test_basic_priority();
    test_equal_priority();
    test_set_priority_runtime();
    test_set_priority_invalid();

    printf("[test_priority] all tests passed.\n");
    return 0;
}
