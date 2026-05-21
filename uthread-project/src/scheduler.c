#include "uthread.h"

#include <stddef.h>

/* ── Ready queue ─────────────────────────────────────────────────────────── */

uthread_t *ready_queue_head = NULL; /* front of the FIFO/RR/Priority queue */
uthread_t *ready_queue_tail = NULL;

/* ── Forward declarations ────────────────────────────────────────────────── */

/* TODO Stage 4: schedule()            — dispatcher, calls swapcontext */
/* TODO Stage 5: fifo_next()           — dequeue head */
/* TODO Stage 5: rr_enqueue_current()  — put current thread at tail */
/* TODO Stage 6: priority_next()       — scan queue for min-priority thread */
