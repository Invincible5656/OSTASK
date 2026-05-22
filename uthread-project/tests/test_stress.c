#include <stdio.h>
#include <assert.h>
#include "uthread.h"

#define N_THREADS 32

/* ── stress: N threads each yield once, then exit ───────────────────────── */

static int run_count = 0;

static void *stress_worker(void *arg)
{
    (void)arg;
    uthread_yield();
    __sync_fetch_and_add(&run_count, 1);
    return NULL;
}

static void test_many_threads(void)
{
    run_count = 0;
    uthread_init(UTHREAD_SCHED_RR);
    for (int i = 0; i < N_THREADS; i++)
        uthread_create(NULL, stress_worker, NULL, 0);
    uthread_start();

    assert(run_count == N_THREADS);
    printf("  [PASS] %d threads ran and exited cleanly\n", N_THREADS);
}

/* ── stress: create, delete half, run the rest ───────────────────────────── */

static int del_run_count = 0;

static void *del_worker(void *arg)
{
    (void)arg;
    __sync_fetch_and_add(&del_run_count, 1);
    return NULL;
}

static void test_create_delete(void)
{
    del_run_count = 0;
    uthread_init(UTHREAD_SCHED_FIFO);

    int tids[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
        uthread_create(&tids[i], del_worker, NULL, 0);

    /* Delete every other thread before scheduling. */
    int deleted = 0;
    for (int i = 0; i < N_THREADS; i += 2) {
        assert(uthread_delete(tids[i]) == 0);
        deleted++;
    }

    uthread_start();

    assert(del_run_count == N_THREADS - deleted);
    printf("  [PASS] deleted %d / %d threads; %d ran correctly\n",
           deleted, N_THREADS, del_run_count);
}

/* ── stress: FIFO / RR / Priority all complete without crash ─────────────── */

static void test_all_policies(void)
{
    uthread_sched_policy_t policies[] = {
        UTHREAD_SCHED_FIFO,
        UTHREAD_SCHED_RR,
        UTHREAD_SCHED_PRIORITY
    };
    const char *names[] = { "FIFO", "RR", "Priority" };

    for (int p = 0; p < 3; p++) {
        run_count = 0;
        uthread_init(policies[p]);
        for (int i = 0; i < 16; i++)
            uthread_create(NULL, stress_worker, NULL, i % 4);
        uthread_start();
        assert(run_count == 16);
        printf("  [PASS] 16 threads under %s scheduling\n", names[p]);
    }
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_stress] stress tests\n");

    test_many_threads();
    test_create_delete();
    test_all_policies();

    printf("[test_stress] all tests passed.\n");
    return 0;
}
