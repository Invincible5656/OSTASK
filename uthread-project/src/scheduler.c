#include "uthread.h"

#include <stddef.h>

/* ── Ready queue globals ─────────────────────────────────────────────────── */

uthread_t *ready_queue_head = NULL;
uthread_t *ready_queue_tail = NULL;

/* ── Scheduling-done flag ────────────────────────────────────────────────── */

/*
 * Set to 1 by schedule() when the ready queue drains.
 * uthread_start() reads this after resuming via swapcontext to know
 * whether all user threads have finished and it should return.
 */
int sched_done = 0;

/* ── schedule ────────────────────────────────────────────────────────────── */

/*
 * Pick the next runnable thread and switch to it.
 *
 * FIFO / RR: dequeue from head.
 *   - FIFO threads run until they yield or exit (no auto-requeue).
 *   - RR threads requeue themselves at the tail in uthread_yield().
 *   Both policies use the same dequeue-from-head logic here.
 *
 * Priority: scan the whole queue for the lowest priority number.
 *   Implemented in Stage 6; falls back to head-dequeue until then.
 *
 * When the queue is empty: set sched_done and swap back to the main
 * thread's saved context so uthread_start() can return cleanly.
 */
void schedule(void)
{
    uthread_t *next = NULL;

    if (sched_policy == UTHREAD_SCHED_PRIORITY) {
        /* Stage 6: scan for minimum-priority READY thread. */
        uthread_t *best = NULL;
        uthread_t *cur  = ready_queue_head;
        while (cur != NULL) {
            if (best == NULL || cur->priority < best->priority)
                best = cur;
            cur = cur->next;
        }
        if (best != NULL)
            next = queue_remove(&ready_queue_head, &ready_queue_tail,
                                best->tid);
    } else {
        /* FIFO and RR: simply dequeue the head. */
        next = queue_dequeue(&ready_queue_head, &ready_queue_tail);
    }

    if (next == NULL) {
        /*
         * No runnable thread left.  Swap back to the main thread's
         * context (saved by uthread_start) so the library call returns.
         */
        sched_done = 1;
        uthread_t *prev = current_thread;
        current_thread        = all_threads; /* main thread is always first */
        all_threads->state    = UTHREAD_RUNNING;
        swapcontext(&prev->context, &all_threads->context);
        return;
    }

    uthread_t *prev = current_thread;
    next->state    = UTHREAD_RUNNING;
    current_thread = next;
    swapcontext(&prev->context, &next->context);
}
