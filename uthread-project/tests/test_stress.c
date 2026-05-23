#include <stdio.h>
#include <assert.h>
#include "uthread.h"

/* 压力测试 */

#define N_THREADS 32

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
    printf("  [PASS] %d 个线程全部正常运行并退出\n", N_THREADS);
}

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

    int deleted = 0;
    for (int i = 0; i < N_THREADS; i += 2) {
        assert(uthread_delete(tids[i]) == 0);
        deleted++;
    }

    uthread_start();

    assert(del_run_count == N_THREADS - deleted);
    printf("  [PASS] 删除 %d / %d 个线程，%d 个正常运行\n",
           deleted, N_THREADS, del_run_count);
}

static void test_all_policies(void)
{
    uthread_sched_policy_t policies[] = {
        UTHREAD_SCHED_FIFO, UTHREAD_SCHED_RR, UTHREAD_SCHED_PRIORITY
    };
    const char *names[] = { "FIFO", "RR", "Priority" };

    for (int p = 0; p < 3; p++) {
        run_count = 0;
        uthread_init(policies[p]);
        for (int i = 0; i < 16; i++)
            uthread_create(NULL, stress_worker, NULL, i % 4);
        uthread_start();
        assert(run_count == 16);
        printf("  [PASS] %s 策略下 16 线程全部完成\n", names[p]);
    }
}

int main(void)
{
    printf("[test_stress] 压力测试\n");

    test_many_threads();
    test_create_delete();
    test_all_policies();

    printf("[test_stress] 全部通过\n");
    return 0;
}
