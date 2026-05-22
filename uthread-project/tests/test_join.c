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

/* ════════════════════════════════════════════════════════════════════════════
 * Test 1: join blocks caller until target exits
 *
 * FIFO queue starts [JOINER, WORKER].
 * JOINER runs first, calls join(WORKER) → WORKER not done → JOINER blocks.
 * WORKER runs → logs "W" → exits → wakes JOINER.
 * JOINER resumes → logs "J".
 * Expected: W J
 * ═══════════════════════════════════════════════════════════════════════════*/

static int worker_tid_g;

static void *worker(void *arg)   { (void)arg; log_event("W"); return NULL; }

static void *joiner(void *arg)
{
    (void)arg;
    uthread_join(worker_tid_g, NULL); /* blocks here until worker exits */
    log_event("J");
    return NULL;
}

static void test_join_blocks(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL,          joiner, NULL, 0); /* runs first */
    uthread_create(&worker_tid_g, worker, NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "W J ") == 0);
    printf("  [PASS] join blocks caller: %s\n", log_buf);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Test 2: join on already-exited thread returns immediately
 *
 * FIFO queue starts [WORKER, JOINER].
 * WORKER runs and exits first.
 * JOINER runs, calls join(WORKER) → already EXITED → returns at once.
 * Expected: W J_imm
 * ═══════════════════════════════════════════════════════════════════════════*/

static int worker2_tid_g;

static void *worker2(void *arg)  { (void)arg; log_event("W2"); return NULL; }

static void *joiner2(void *arg)
{
    (void)arg;
    int rc = uthread_join(worker2_tid_g, NULL);
    assert(rc == 0);
    log_event("J2_imm");
    return NULL;
}

static void test_join_already_exited(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(&worker2_tid_g, worker2,  NULL, 0); /* exits first */
    uthread_create(NULL,           joiner2,  NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "W2 J2_imm ") == 0);
    printf("  [PASS] join on exited thread returns immediately: %s\n", log_buf);
}

/* ════════════════════════════════════════════════════════════════════════════
 * Test 3: retval is passed through join correctly
 * ═══════════════════════════════════════════════════════════════════════════*/

static int retval_worker_tid_g;

static void *retval_worker(void *arg)
{
    (void)arg;
    uthread_exit((void *)42);
    return NULL; /* unreachable */
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
    uthread_create(NULL,                retval_joiner, NULL, 0);
    uthread_create(&retval_worker_tid_g, retval_worker, NULL, 0);
    uthread_start();

    assert(strcmp(log_buf, "RV_OK ") == 0);
    printf("  [PASS] retval passed through join correctly\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Test 4: uthread_delete removes a READY thread; it never runs
 * ═══════════════════════════════════════════════════════════════════════════*/

static void *should_not_run(void *arg)
{
    (void)arg;
    log_event("DELETED_THREAD_RAN"); /* must never appear */
    return NULL;
}

static void test_delete_ready(void)
{
    log_buf[0] = '\0'; log_pos = 0;

    uthread_init(UTHREAD_SCHED_FIFO);
    int del_tid;
    uthread_create(&del_tid, should_not_run, NULL, 0);
    uthread_create(NULL,     worker,         NULL, 0);

    /* Delete the thread before scheduling starts. */
    assert(uthread_delete(del_tid) == 0);
    uthread_start();

    assert(strstr(log_buf, "DELETED_THREAD_RAN") == NULL);
    assert(strstr(log_buf, "W") != NULL);
    printf("  [PASS] deleted READY thread never runs\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Test 5: uthread_delete rejects invalid and RUNNING threads
 * ═══════════════════════════════════════════════════════════════════════════*/

static void test_delete_invalid(void)
{
    uthread_init(UTHREAD_SCHED_FIFO);

    /* Non-existent tid. */
    assert(uthread_delete(9999) == -1);

    /* Cannot delete the currently RUNNING main thread (tid 0). */
    assert(uthread_delete(all_threads->tid) == -1);

    printf("  [PASS] delete rejects unknown tid and RUNNING thread\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 * Test 6: uthread_list prints without crashing
 * ═══════════════════════════════════════════════════════════════════════════*/

static void test_list(void)
{
    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, worker, NULL, 1);
    uthread_create(NULL, worker, NULL, 2);

    printf("  --- uthread_list output ---\n");
    uthread_list();
    printf("  ---------------------------\n");
    printf("  [PASS] uthread_list prints without crash\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_join] join / delete / list tests\n");

    test_join_blocks();
    test_join_already_exited();
    test_join_retval();
    test_delete_ready();
    test_delete_invalid();
    test_list();

    printf("[test_join] all tests passed.\n");
    return 0;
}
