#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "uthread.h"

/* 队列模块单元测试 */

static uthread_t *make_thread(int tid)
{
    uthread_t *t = calloc(1, sizeof(uthread_t));
    assert(t != NULL);
    t->tid   = tid;
    t->state = UTHREAD_READY;
    return t;
}

static void test_enqueue_dequeue(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1), *t2 = make_thread(2), *t3 = make_thread(3);

    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);
    queue_enqueue(&head, &tail, t3);

    assert(queue_dequeue(&head, &tail) == t1);
    assert(queue_dequeue(&head, &tail) == t2);
    assert(queue_dequeue(&head, &tail) == t3);
    assert(queue_dequeue(&head, &tail) == NULL);

    free(t1); free(t2); free(t3);
    printf("  [PASS] 入队 / 出队顺序\n");
}

static void test_dequeue_empty(void)
{
    uthread_t *head = NULL, *tail = NULL;
    assert(queue_dequeue(&head, &tail) == NULL);
    printf("  [PASS] 空队列出队返回 NULL\n");
}

static void test_find(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(10), *t2 = make_thread(20);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);

    assert(queue_find(head, 10) == t1);
    assert(queue_find(head, 20) == t2);
    assert(queue_find(head, 99) == NULL);

    free(queue_dequeue(&head, &tail));
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] 按 tid 查找\n");
}

static void test_remove_head(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1), *t2 = make_thread(2);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);

    assert(queue_remove(&head, &tail, 1) == t1);
    assert(head == t2 && tail == t2);

    free(t1);
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] 删除头节点\n");
}

static void test_remove_tail(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1), *t2 = make_thread(2);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);

    assert(queue_remove(&head, &tail, 2) == t2);
    assert(head == t1 && tail == t1);

    free(t2);
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] 删除尾节点\n");
}

static void test_remove_middle(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1), *t2 = make_thread(2), *t3 = make_thread(3);
    queue_enqueue(&head, &tail, t1);
    queue_enqueue(&head, &tail, t2);
    queue_enqueue(&head, &tail, t3);

    assert(queue_remove(&head, &tail, 2) == t2);
    assert(head == t1 && tail == t3);
    assert(t1->next == t3);

    free(t2);
    free(queue_dequeue(&head, &tail));
    free(queue_dequeue(&head, &tail));
    printf("  [PASS] 删除中间节点\n");
}

static void test_remove_not_found(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1);
    queue_enqueue(&head, &tail, t1);

    assert(queue_remove(&head, &tail, 99) == NULL);

    free(queue_dequeue(&head, &tail));
    printf("  [PASS] 删除不存在的 tid 返回 NULL\n");
}

static void test_single_element(void)
{
    uthread_t *head = NULL, *tail = NULL;
    uthread_t *t1 = make_thread(1);
    queue_enqueue(&head, &tail, t1);

    assert(queue_remove(&head, &tail, 1) == t1);
    assert(head == NULL && tail == NULL);

    free(t1);
    printf("  [PASS] 删除唯一元素后队列为空\n");
}

int main(void)
{
    printf("[test_queue] 队列模块单元测试\n");

    test_enqueue_dequeue();
    test_dequeue_empty();
    test_find();
    test_remove_head();
    test_remove_tail();
    test_remove_middle();
    test_remove_not_found();
    test_single_element();

    printf("[test_queue] 全部通过\n");
    return 0;
}
