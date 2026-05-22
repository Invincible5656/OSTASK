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

static void *fifo_thread(void *arg)
{
    const char *name = (const char *)arg;
    log_event(name);
    /* No yield: FIFO thread runs to completion without releasing CPU. */
    return NULL;
}

static void *yield_thread(void *arg)
{
    const char *name = (const char *)arg;
    char step[8];

    snprintf(step, sizeof(step), "%s1", name);
    log_event(step);

    uthread_yield();

    snprintf(step, sizeof(step), "%s2", name);
    log_event(step);

    return NULL;
}

/* ── test: FIFO threads run in creation order without interleaving ────────── */

static void test_fifo_order(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, fifo_thread, "T1", 0);
    uthread_create(NULL, fifo_thread, "T2", 0);
    uthread_create(NULL, fifo_thread, "T3", 0);
    uthread_start();

    /*
     * FIFO: each thread runs to exit before the next one starts.
     * Expected: T1 T2 T3
     */
    assert(strcmp(log_buf, "T1 T2 T3 ") == 0);
    printf("  [PASS] FIFO order (no yield): %s\n", log_buf);
}

/* ── test: FIFO with yield still preserves queue order ───────────────────── */

static void test_fifo_yield_order(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, yield_thread, "A", 0);
    uthread_create(NULL, yield_thread, "B", 0);
    uthread_create(NULL, yield_thread, "C", 0);
    uthread_start();

    /*
     * Queue at start: [A, B, C]
     * A runs → A1, yield → queue [B, C, A]
     * B runs → B1, yield → queue [C, A, B]
     * C runs → C1, yield → queue [A, B, C]
     * A runs → A2, exit  → queue [B, C]
     * B runs → B2, exit  → queue [C]
     * C runs → C2, exit  → queue []
     * Expected: A1 B1 C1 A2 B2 C2
     */
    assert(strcmp(log_buf, "A1 B1 C1 A2 B2 C2 ") == 0);
    printf("  [PASS] FIFO with yield: %s\n", log_buf);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_fifo] FIFO scheduling tests\n");

    test_fifo_order();
    test_fifo_yield_order();

    printf("[test_fifo] all tests passed.\n");
    return 0;
}
