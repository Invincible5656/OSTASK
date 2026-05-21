#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "uthread.h"

/* ── dummy thread function ───────────────────────────────────────────────── */

static void *dummy(void *arg)
{
    (void)arg;
    return NULL;
}

/* ── Stage 3: verify uthread_init and uthread_create ────────────────────── */

static void test_init(void)
{
    assert(uthread_init(UTHREAD_SCHED_FIFO) == 0);

    /* main thread should be tid 0, RUNNING */
    assert(current_thread         != NULL);
    assert(current_thread->tid    == 0);
    assert(current_thread->state  == UTHREAD_RUNNING);
    assert(current_thread->pid    == getpid());
    assert(current_thread->stack  == NULL); /* main uses process stack */

    /* all_threads list has exactly one entry */
    assert(all_threads            == current_thread);
    assert(all_threads->all_next  == NULL);

    /* ready queue is still empty */
    assert(ready_queue_head       == NULL);

    printf("  [PASS] uthread_init: main thread TCB correct\n");
}

static void test_create(void)
{
    int tid1, tid2, tid3;

    assert(uthread_create(&tid1, dummy, NULL,  2) == 0);
    assert(uthread_create(&tid2, dummy, NULL,  1) == 0);
    assert(uthread_create(&tid3, dummy, (void *)"hello", 3) == 0);

    /* tids are sequential starting from 1 */
    assert(tid1 == 1);
    assert(tid2 == 2);
    assert(tid3 == 3);
    printf("  [PASS] uthread_create: sequential tids (%d, %d, %d)\n",
           tid1, tid2, tid3);

    /* all three threads are in the all_threads list */
    uthread_t *t = all_threads->all_next; /* skip main */
    assert(t != NULL && t->tid == 1 && t->state == UTHREAD_READY);
    t = t->all_next;
    assert(t != NULL && t->tid == 2 && t->priority == 1);
    t = t->all_next;
    assert(t != NULL && t->tid == 3);
    assert(t->all_next == NULL);
    printf("  [PASS] uthread_create: all_threads list has 4 entries\n");

    /* ready queue has the three new threads in creation order */
    assert(ready_queue_head       != NULL);
    assert(ready_queue_head->tid  == 1);
    assert(ready_queue_tail->tid  == 3);
    printf("  [PASS] uthread_create: ready queue head=%d tail=%d\n",
           ready_queue_head->tid, ready_queue_tail->tid);

    /* each thread has its own private stack */
    uthread_t *t1 = queue_find(ready_queue_head, tid1);
    uthread_t *t2 = queue_find(ready_queue_head, tid2);
    assert(t1 != NULL && t1->stack != NULL);
    assert(t2 != NULL && t2->stack != NULL);
    assert(t1->stack != t2->stack);
    printf("  [PASS] uthread_create: each thread has a unique private stack\n");

    /* pid and create_time are filled in */
    assert(t1->pid == getpid());
    assert(t1->create_time > 0);
    printf("  [PASS] uthread_create: pid and create_time populated\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_basic] Stage 3: uthread_init + uthread_create\n");

    test_init();
    test_create();

    printf("[test_basic] all Stage 3 tests passed.\n");
    return 0;
}
