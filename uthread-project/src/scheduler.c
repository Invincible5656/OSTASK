#include "uthread.h"

#include <stddef.h>

/* ── Ready queue globals ─────────────────────────────────────────────────── */

uthread_t *ready_queue_head = NULL;
uthread_t *ready_queue_tail = NULL;

/* ── schedule ────────────────────────────────────────────────────────────── */

void schedule(void)
{
    /* TODO Stage 4: pick next thread via FIFO / RR / Priority,
       update states, call swapcontext. */
}

/* ── Policy-specific helpers (Stage 5 / 6) ──────────────────────────────── */

/* TODO Stage 5: fifo_next()          — dequeue head */
/* TODO Stage 5: rr_enqueue_current() — put current thread at tail */
/* TODO Stage 6: priority_next()      — scan queue for min-priority thread */
