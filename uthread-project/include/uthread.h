#ifndef UTHREAD_H
#define UTHREAD_H

#include <ucontext.h>
#include <time.h>
#include <sys/types.h>

/* ── Constants ─────────────────────────────────────────────────────────── */

#define UTHREAD_STACK_SIZE   (64 * 1024)  /* 64 KB per thread stack */
#define UTHREAD_MAX_THREADS  64

/* ── Enumerations ───────────────────────────────────────────────────────── */

typedef enum {
    UTHREAD_READY,
    UTHREAD_RUNNING,
    UTHREAD_BLOCKED,
    UTHREAD_EXITED
} uthread_state_t;

typedef enum {
    UTHREAD_SCHED_FIFO,
    UTHREAD_SCHED_RR,
    UTHREAD_SCHED_PRIORITY
} uthread_sched_policy_t;

/* ── Thread Control Block ───────────────────────────────────────────────── */

typedef struct uthread {
    int                  tid;            /* unique thread id, library-assigned */
    int                  priority;       /* lower number = higher priority */
    pid_t                pid;            /* owning process id */
    time_t               create_time;   /* timestamp from time(NULL) */
    uthread_state_t      state;
    ucontext_t           context;
    void                *stack;          /* privately allocated stack */
    void                *(*func)(void *);/* thread entry point */
    void                *arg;            /* argument passed to func */
    void                *retval;         /* value passed to uthread_exit */
    struct uthread      *waiting_thread; /* thread blocked on uthread_join */
    struct uthread      *next;           /* ready-queue link */
    struct uthread      *all_next;       /* global all-threads list link */
} uthread_t;

/* ── Global state (definitions in uthread.c / scheduler.c) ─────────────── */

extern uthread_t            *current_thread;
extern uthread_t            *all_threads;
extern uthread_sched_policy_t sched_policy;
extern int                   next_tid;
extern uthread_t            *ready_queue_head;
extern uthread_t            *ready_queue_tail;

/* ── Public API ─────────────────────────────────────────────────────────── */

/* Initialize the thread library with the given scheduling policy.
   Must be called before any other uthread_* function. */
int  uthread_init(uthread_sched_policy_t policy);

/* Start the scheduler loop; returns only when all threads have exited. */
void uthread_start(void);

/* Create a new thread. Writes the new tid into *tid.
   Returns 0 on success, -1 on error. */
int  uthread_create(int *tid,
                    void *(*start_routine)(void *),
                    void *arg,
                    int   priority);

/* Voluntarily yield the CPU to the next ready thread. */
void uthread_yield(void);

/* Terminate the calling thread, saving retval for uthread_join callers. */
void uthread_exit(void *retval);

/* Block the calling thread until thread tid exits.
   If retval is non-NULL, the target thread's return value is stored there.
   Returns 0 on success, -1 if tid is invalid or already deleted. */
int  uthread_join(int tid, void **retval);

/* Delete a READY or EXITED thread and free its resources.
   Returns 0 on success, -1 if the thread is RUNNING or not found. */
int  uthread_delete(int tid);

/* Change the priority of thread tid.
   Returns 0 on success, -1 if tid is not found. */
int  uthread_set_priority(int tid, int priority);

/* Print a formatted table of all threads (tid, pid, priority, state,
   create_time) to stdout. */
void uthread_list(void);

/* ── Internal scheduler API (used across translation units) ─────────────── */

/* Select and switch to the next runnable thread.
   Called by yield, exit, join, and start. */
void schedule(void);

/* ── Internal queue API (intrusive singly-linked list via uthread_t.next) ── */

/* Append t to the tail of the queue. */
void       queue_enqueue(uthread_t **head, uthread_t **tail, uthread_t *t);

/* Remove and return the head node; returns NULL if the queue is empty. */
uthread_t *queue_dequeue(uthread_t **head, uthread_t **tail);

/* Find the node with the given tid; returns NULL if not found. */
uthread_t *queue_find(uthread_t *head, int tid);

/* Remove the node with the given tid and return it; returns NULL if not found. */
uthread_t *queue_remove(uthread_t **head, uthread_t **tail, int tid);

#endif /* UTHREAD_H */
