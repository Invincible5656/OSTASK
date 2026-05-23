#include "uthread.h"

#include <stddef.h>

/* 基于 uthread_t.next 的侵入式单链表，
   enqueue/dequeue 为 O(1)，find/remove 为 O(n) */

void queue_enqueue(uthread_t **head, uthread_t **tail, uthread_t *t)
{
    t->next = NULL;
    if (*tail == NULL) {
        *head = *tail = t;
    } else {
        (*tail)->next = t;
        *tail = t;
    }
}

uthread_t *queue_dequeue(uthread_t **head, uthread_t **tail)
{
    if (*head == NULL) return NULL;

    uthread_t *t = *head;
    *head = t->next;
    if (*head == NULL) *tail = NULL;
    t->next = NULL;
    return t;
}

uthread_t *queue_find(uthread_t *head, int tid)
{
    for (uthread_t *t = head; t != NULL; t = t->next) {
        if (t->tid == tid) return t;
    }
    return NULL;
}

uthread_t *queue_remove(uthread_t **head, uthread_t **tail, int tid)
{
    uthread_t *prev = NULL, *cur = *head;
    while (cur != NULL) {
        if (cur->tid == tid) {
            if (prev == NULL) *head = cur->next;
            else              prev->next = cur->next;
            if (cur->next == NULL) *tail = prev;
            cur->next = NULL;
            return cur;
        }
        prev = cur;
        cur  = cur->next;
    }
    return NULL;
}
