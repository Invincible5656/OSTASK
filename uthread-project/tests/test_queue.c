#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "uthread.h"

/* ── helpers ─────────────────────────────────────────────────────────────── */

static uthread_t *make_thread(int tid)
{
    uthread_t *t = calloc(1, sizeof(uthread_t));
    assert(t != NULL);
    t->tid   = tid;
    t->state = UTHREAD_READY;
    t->next  = NULL;
    return t;
}

/* ── test cases ──────────────────────────────────────────────────────────── */

static void test_enqueue_dequeue(void)
{
    uthread_t *head = NULL, *tail = NULL;

    uthread_t *t1 = make_thread(1);
    uthread_t *t2 = make_thread(2);
    uthread_t *t3 = make_thread(3);

    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);
    queue_enqueue(&head, &tail, t3);

    assert(queue_dequeue(&head, &tail) == t1);
    assert(queue_dequeue(&head, &tail) == t2);
    assert(queue_dequeue(&head, &tail) == t3);
    assert(queue_dequeue(&head, &tail) == NULL); /* empty */

    free(t1); free(t2); free(t3);
    printf("  [PASS] enqueue / dequeue order\n");
}

static void test_dequeue_empty(void)
{
    uthread_t *head = NULL, *tail = NULL;
    assert(queue_dequeue(&head, &tail) == NULL);
    printf("  [PASS] dequeue on empty queue returns NULL\n");
}

static void test_find(void)
{
    uthread_t *head = NULL, *tail = NULL;

    uthread_t *t1 = make_thread(10);
    uthread_t *t2 = make_thread(20);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);

    assert(queue_find(head, 10) == t1);
    assert(queue_find(head, 20) == t2);
    assert(queue_find(head, 99) == NULL);

    free(queue_dequeue(&head, &tail));
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] find by tid\n");
}

static void test_remove_head(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1);
    uthread_t *t2 = make_thread(2);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);

    uthread_t *removed = queue_remove(&head, &tail, 1);
    assert(removed == t1);
    assert(head == t2);
    assert(tail == t2);

    free(t1);
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] remove head node\n");
}

static void test_remove_tail(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1);
    uthread_t *t2 = make_thread(2);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);

    uthread_t *removed = queue_remove(&head, &tail, 2);
    assert(removed == t2);
    assert(head == t1);
    assert(tail == t1);

    free(t2);
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] remove tail node\n");
}

static void test_remove_middle(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1);
    uthread_t *t2 = make_thread(2);
    uthread_t *t3 = make_thread(3);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);
    queue_enqueue(&head, &tail, t3);

    uthread_t *removed = queue_remove(&head, &tail, 2);
    assert(removed == t2);
    assert(head == t1);
    assert(tail == t3);
    assert(t1->next == t3);

    free(t2);
    free(queue_dequeue(&head, &tail));
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] remove middle node\n");
}

static void test_remove_not_found(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1);
    queue_enqueue(&head, &tail, t1);

    assert(queue_remove(&head, &tail, 99) == NULL);

    free(queue_dequeue(&head, &tail));
    printf("  [PASS] remove non-existent tid returns NULL\n");
}

static void test_single_element(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1);
    queue_enqueue(&head, &tail, t1);

    uthread_t *removed = queue_remove(&head, &tail, 1);
    assert(removed == t1);
    assert(head == NULL);
    assert(tail == NULL);

    free(t1);
    printf("  [PASS] remove sole element leaves empty queue\n");
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("[test_queue] running queue unit tests...\n");

    test_enqueue_dequeue();
    test_dequeue_empty();
    test_find();
    test_remove_head();
    test_remove_tail();
    test_remove_middle();
    test_remove_not_found();
    test_single_element();

    printf("[test_queue] all tests passed.\n");
    return 0;
}
