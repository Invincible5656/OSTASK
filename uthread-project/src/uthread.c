#include "uthread.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* ── Global state ────────────────────────────────────────────────────────── */

uthread_t            *current_thread = NULL;
uthread_t            *all_threads    = NULL;
uthread_sched_policy_t sched_policy  = UTHREAD_SCHED_FIFO;
int                   next_tid       = 0;

/* ── Internal thread entry wrapper ──────────────────────────────────────── */

/*
 * makecontext() can only pass int arguments, but a pointer is 64 bits on
 * modern systems.  We split the TCB pointer into two uint32_t halves and
 * reconstruct it here so the thread can find its own TCB.
 */
static void thread_wrapper(uint32_t hi, uint32_t lo)
{
    uthread_t *t = (uthread_t *)( ((uintptr_t)hi << 32) | (uintptr_t)lo );
    void *retval = t->func(t->arg);
    uthread_exit(retval);
}

/* ── uthread_init ────────────────────────────────────────────────────────── */

int uthread_init(uthread_sched_policy_t policy)
{
    sched_policy = policy;

    uthread_t *main_t = calloc(1, sizeof(uthread_t));
    if (main_t == NULL)
        return -1;

    main_t->tid         = next_tid++;
    main_t->priority    = 0;
    main_t->pid         = getpid();
    main_t->create_time = time(NULL);
    main_t->state       = UTHREAD_RUNNING;
    /* main thread uses the process stack — no private allocation needed */

    current_thread = main_t;
    all_threads    = main_t;
    return 0;
}

/* ── uthread_create ──────────────────────────────────────────────────────── */

int uthread_create(int *tid,
                   void *(*start_routine)(void *),
                   void *arg,
                   int   priority)
{
    uthread_t *t = calloc(1, sizeof(uthread_t));
    if (t == NULL)
        return -1;

    t->stack = malloc(UTHREAD_STACK_SIZE);
    if (t->stack == NULL) {
        free(t);
        return -1;
    }

    t->tid         = next_tid++;
    t->priority    = priority;
    t->pid         = getpid();
    t->create_time = time(NULL);
    t->state       = UTHREAD_READY;
    t->func        = start_routine;
    t->arg         = arg;

    /* Build an executable context for this thread. */
    if (getcontext(&t->context) == -1) {
        free(t->stack);
        free(t);
        return -1;
    }
    t->context.uc_stack.ss_sp   = t->stack;
    t->context.uc_stack.ss_size = UTHREAD_STACK_SIZE;
    t->context.uc_link          = NULL; /* thread_wrapper calls uthread_exit */

    uintptr_t ptr = (uintptr_t)t;
    makecontext(&t->context, (void (*)(void))thread_wrapper, 2,
                (uint32_t)(ptr >> 32),
                (uint32_t)(ptr & 0xFFFFFFFFu));

    /* Append to the global all-threads list. */
    uthread_t *cur = all_threads;
    while (cur->all_next != NULL)
        cur = cur->all_next;
    cur->all_next = t;

    /* Enqueue in the ready queue. */
    queue_enqueue(&ready_queue_head, &ready_queue_tail, t);

    if (tid != NULL)
        *tid = t->tid;

    return 0;
}

/* ── uthread_yield (Stage 4) ─────────────────────────────────────────────── */

void uthread_yield(void)
{
    /* TODO Stage 4 */
}

/* ── uthread_exit (Stage 4) ──────────────────────────────────────────────── */

void uthread_exit(void *retval)
{
    (void)retval;
    /* TODO Stage 4 */
}

/* ── uthread_start (Stage 4) ─────────────────────────────────────────────── */

void uthread_start(void)
{
    /* TODO Stage 4 */
}

/* ── uthread_join (Stage 7) ──────────────────────────────────────────────── */

int uthread_join(int tid, void **retval)
{
    (void)tid; (void)retval;
    /* TODO Stage 7 */
    return -1;
}

/* ── uthread_delete (Stage 7) ────────────────────────────────────────────── */

int uthread_delete(int tid)
{
    (void)tid;
    /* TODO Stage 7 */
    return -1;
}

/* ── uthread_set_priority (Stage 6) ─────────────────────────────────────── */

int uthread_set_priority(int tid, int priority)
{
    (void)tid; (void)priority;
    /* TODO Stage 6 */
    return -1;
}

/* ── uthread_list (Stage 7) ──────────────────────────────────────────────── */

void uthread_list(void)
{
    /* TODO Stage 7 */
}
