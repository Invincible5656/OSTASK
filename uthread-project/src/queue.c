#include "uthread.h"

#include <stddef.h>

/*
 * Intrusive singly-linked queue using uthread_t.next as the link pointer.
 * All operations are O(1) except queue_find and queue_remove which are O(n).
 */

/* Append t to the tail of the queue. */
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

/* Remove and return the head node; returns NULL if the queue is empty. */
uthread_t *queue_dequeue(uthread_t **head, uthread_t **tail)
{
    if (*head == NULL)
        return NULL;

    uthread_t *t = *head;
    *head = t->next;
    if (*head == NULL)
        *tail = NULL;

    t->next = NULL;
    return t;
}

/* Find the node with the given tid; returns NULL if not found. */
uthread_t *queue_find(uthread_t *head, int tid)
{
    for (uthread_t *t = head; t != NULL; t = t->next) {
        if (t->tid == tid)
            return t;
    }
    return NULL;
}

/* Remove the node with the given tid and return it; returns NULL if not found.
   Handles removal at head, middle, and tail. */
uthread_t *queue_remove(uthread_t **head, uthread_t **tail, int tid)
{
    uthread_t *prev = NULL;
    uthread_t *cur  = *head;

    while (cur != NULL) {
        if (cur->tid == tid) {
            if (prev == NULL)
                *head = cur->next;      /* removing head */
            else
                prev->next = cur->next; /* removing middle or tail */

            if (cur->next == NULL)
                *tail = prev;           /* removed node was the tail */

            cur->next = NULL;
            return cur;
        }
        prev = cur;
        cur  = cur->next;
    }
    return NULL;
}
