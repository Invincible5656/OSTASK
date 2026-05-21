#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uthread.h"

/* ── shared event log ────────────────────────────────────────────────────── */

static char log_buf[256];
static int  log_pos = 0;

static void log_event(const char *ev)
{
    log_pos += snprintf(log_buf + log_pos,
                        sizeof(log_buf) - log_pos, "%s ", ev);
}

/* ── thread functions ────────────────────────────────────────────────────── */

static void *thread_a(void *arg)
{
    (void)arg;
    log_event("A1");
    uthread_yield();
    log_event("A2");
    return NULL;
}

static void *thread_b(void *arg)
{
    (void)arg;
    log_event("B1");
    uthread_yield();
    log_event("B2");
    return NULL;
}

static void *thread_single(void *arg)
{
    (void)arg;
    log_event("S");
    return NULL;
}

/* ── test: two threads round-robin via yield ─────────────────────────────── */

static void test_yield_order(void)
{
    log_buf[0] = '\0';
    log_pos    = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, thread_a, NULL, 0);
    uthread_create(NULL, thread_b, NULL, 0);
    uthread_start();

    /*
     * Expected interleaving with FIFO + yield-requeue:
     *   A starts → A1 → yield (A→tail, queue=[B,A])
     *   B starts → B1 → yield (B→tail, queue=[A,B])
     *   A resumes → A2 → exit  (queue=[B])
     *   B resumes → B2 → exit  (queue=[])
     */
    assert(strcmp(log_buf, "A1 B1 A2 B2 ") == 0);
    printf("  [PASS] yield interleaving: %s\n", log_buf);
}

/* ── test: thread exit without yield ────────────────────────────────────── */

static void test_exit_no_yield(void)
{
    log_buf[0] = '\0';
    log_pos    = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, thread_single, NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "S ") == 0);
    printf("  [PASS] single thread exit: %s\n", log_buf);
}

/* ── test: uthread_start returns when queue is empty from the start ──────── */

static void test_start_empty(void)
{
    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_start(); /* should return immediately, no crash */
    printf("  [PASS] uthread_start with empty queue returns cleanly\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_basic] Stage 4: yield, exit, context switching\n");

    test_yield_order();
    test_exit_no_yield();
    test_start_empty();

    printf("[test_basic] all Stage 4 tests passed.\n");
    return 0;
}
