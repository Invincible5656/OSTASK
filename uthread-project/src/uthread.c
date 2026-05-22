#include "uthread.h"

#include <stdint.h>

extern int sched_done; /* defined in scheduler.c */
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

/* ── uthread_yield ───────────────────────────────────────────────────────── */

void uthread_yield(void)
{
    current_thread->state = UTHREAD_READY;
    queue_enqueue(&ready_queue_head, &ready_queue_tail, current_thread);
    schedule();
    /* Execution resumes here when this thread is next dispatched. */
}

/* ── uthread_exit ────────────────────────────────────────────────────────── */

void uthread_exit(void *retval)
{
    current_thread->retval = retval;
    current_thread->state  = UTHREAD_EXITED;

    /* Wake any thread blocked in uthread_join() on us (Stage 7). */
    if (current_thread->waiting_thread != NULL) {
        uthread_t *waiter = current_thread->waiting_thread;
        waiter->state = UTHREAD_READY;
        queue_enqueue(&ready_queue_head, &ready_queue_tail, waiter);
        current_thread->waiting_thread = NULL;
    }

    schedule();
    /* Never reached: an exited thread is never re-dispatched. */
}

/* ── uthread_start ───────────────────────────────────────────────────────── */

/*
 * Save the main thread's context here.  When all user threads finish,
 * schedule() sets sched_done=1 and swaps back to this exact point.
 * The flag check then causes uthread_start() to return to the caller.
 */
void uthread_start(void)
{
    sched_done = 0;

    /*
     * getcontext saves the "resume here" address.
     * After schedule() swaps back, we land right after this call,
     * check sched_done, and return.
     */
    getcontext(&all_threads->context);
    if (sched_done)
        return;

    schedule();
}

/* ── uthread_join ────────────────────────────────────────────────────────── */

int uthread_join(int tid, void **retval)
{
    /* Find target thread. */
    uthread_t *target = NULL;
    for (uthread_t *t = all_threads; t != NULL; t = t->all_next) {
        if (t->tid == tid) { target = t; break; }
    }
    if (target == NULL || target == current_thread)
        return -1;

    /* If already exited, collect retval immediately. */
    if (target->state == UTHREAD_EXITED) {
        if (retval != NULL) *retval = target->retval;
        return 0;
    }

    /* Reject a second concurrent joiner on the same target. */
    if (target->waiting_thread != NULL)
        return -1;

    /* Block until target exits.  uthread_exit() will re-enqueue us. */
    target->waiting_thread = current_thread;
    current_thread->state  = UTHREAD_BLOCKED;
    schedule();

    /* Resumed: target is now EXITED. */
    if (retval != NULL) *retval = target->retval;
    return 0;
}

/* ── uthread_delete ──────────────────────────────────────────────────────── */

int uthread_delete(int tid)
{
    /* Find target and its predecessor in the all_threads list. */
    uthread_t *prev   = NULL;
    uthread_t *target = NULL;
    for (uthread_t *t = all_threads; t != NULL; t = t->all_next) {
        if (t->tid == tid) { target = t; break; }
        prev = t;
    }
    if (target == NULL)                              return -1;
    if (target->state == UTHREAD_RUNNING)            return -1;
    if (target->state == UTHREAD_BLOCKED)            return -1;
    /* Prevent use-after-free: another thread is blocked waiting on this one. */
    if (target->waiting_thread != NULL)              return -1;

    /* Remove from ready queue if present. */
    if (target->state == UTHREAD_READY)
        queue_remove(&ready_queue_head, &ready_queue_tail, tid);

    /* Unlink from all_threads list. */
    if (prev != NULL)
        prev->all_next = target->all_next;
    else
        all_threads = target->all_next;

    free(target->stack);
    free(target);
    return 0;
}

/* ── uthread_set_priority (Stage 6) ─────────────────────────────────────── */

int uthread_set_priority(int tid, int priority)
{
    for (uthread_t *t = all_threads; t != NULL; t = t->all_next) {
        if (t->tid == tid) {
            t->priority = priority;
            return 0;
        }
    }
    return -1; /* tid not found */
}

/* ── uthread_list ────────────────────────────────────────────────────────── */

void uthread_list(void)
{
    static const char *state_name[] = {
        "READY", "RUNNING", "BLOCKED", "EXITED"
    };

    printf("%-5s %-8s %-10s %-10s %s\n",
           "TID", "PID", "PRIORITY", "STATE", "CREATED");
    printf("%-5s %-8s %-10s %-10s %s\n",
           "----", "-------", "--------", "---------", "-------------------");

    for (uthread_t *t = all_threads; t != NULL; t = t->all_next) {
        char tbuf[32];
        struct tm *tm_info = localtime(&t->create_time);
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_info);
        printf("%-5d %-8d %-10d %-10s %s\n",
               t->tid, (int)t->pid, t->priority,
               state_name[t->state], tbuf);
    }
}
