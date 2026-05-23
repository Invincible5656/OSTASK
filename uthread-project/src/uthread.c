#include "uthread.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

extern int sched_done;

uthread_t            *current_thread = NULL;
uthread_t            *all_threads    = NULL;
uthread_sched_policy_t sched_policy  = UTHREAD_SCHED_FIFO;
int                   next_tid       = 0;

/* makecontext 只能传 int 参数，64 位指针拆成两半，进入线程时再拼回 TCB 指针 */
static void thread_wrapper(uint32_t hi, uint32_t lo)
{
    uthread_t *t = (uthread_t *)(((uintptr_t)hi << 32) | (uintptr_t)lo);
    void *retval = t->func(t->arg);
    uthread_exit(retval);
}

int uthread_init(uthread_sched_policy_t policy)
{
    sched_policy = policy;

    uthread_t *m = calloc(1, sizeof(uthread_t));
    if (m == NULL) return -1;

    m->tid         = next_tid++;
    m->priority    = 0;
    m->pid         = getpid();
    m->create_time = time(NULL);
    m->state       = UTHREAD_RUNNING;

    current_thread = m;
    all_threads    = m;
    return 0;
}

int uthread_create(int *tid, void *(*start_routine)(void *),
                   void *arg, int priority)
{
    uthread_t *t = calloc(1, sizeof(uthread_t));
    if (t == NULL) return -1;

    t->stack = malloc(UTHREAD_STACK_SIZE);
    if (t->stack == NULL) { free(t); return -1; }

    t->tid         = next_tid++;
    t->priority    = priority;
    t->pid         = getpid();
    t->create_time = time(NULL);
    t->state       = UTHREAD_READY;
    t->func        = start_routine;
    t->arg         = arg;

    if (getcontext(&t->context) == -1) {
        free(t->stack); free(t);
        return -1;
    }
    t->context.uc_stack.ss_sp   = t->stack;
    t->context.uc_stack.ss_size = UTHREAD_STACK_SIZE;
    t->context.uc_link          = NULL;

    uintptr_t p = (uintptr_t)t;
    makecontext(&t->context, (void (*)(void))thread_wrapper, 2,
                (uint32_t)(p >> 32), (uint32_t)(p & 0xFFFFFFFFu));

    /* 挂到全部线程链尾 */
    uthread_t *cur = all_threads;
    while (cur->all_next != NULL) cur = cur->all_next;
    cur->all_next = t;

    queue_enqueue(&ready_queue_head, &ready_queue_tail, t);

    if (tid != NULL) *tid = t->tid;
    return 0;
}

void uthread_yield(void)
{
    current_thread->state = UTHREAD_READY;
    queue_enqueue(&ready_queue_head, &ready_queue_tail, current_thread);
    schedule();
}

void uthread_exit(void *retval)
{
    current_thread->retval = retval;
    current_thread->state  = UTHREAD_EXITED;

    /* 唤醒在本线程上 join 的等待者 */
    if (current_thread->waiting_thread != NULL) {
        uthread_t *w = current_thread->waiting_thread;
        w->state = UTHREAD_READY;
        queue_enqueue(&ready_queue_head, &ready_queue_tail, w);
        current_thread->waiting_thread = NULL;
    }

    schedule();
}

/* 保存主线程返回点；schedule() 检测到队列为空时把 sched_done 置 1
   并切换到此处，flag 检查后函数返回 */
void uthread_start(void)
{
    sched_done = 0;
    getcontext(&all_threads->context);
    if (sched_done) return;
    schedule();
}

int uthread_join(int tid, void **retval)
{
    uthread_t *target = NULL;
    for (uthread_t *t = all_threads; t != NULL; t = t->all_next) {
        if (t->tid == tid) { target = t; break; }
    }
    if (target == NULL || target == current_thread) return -1;

    if (target->state == UTHREAD_EXITED) {
        if (retval != NULL) *retval = target->retval;
        return 0;
    }

    /* 同一目标只允许一个 joiner，避免覆盖前一个等待者 */
    if (target->waiting_thread != NULL) return -1;

    target->waiting_thread = current_thread;
    current_thread->state  = UTHREAD_BLOCKED;
    schedule();

    if (retval != NULL) *retval = target->retval;
    return 0;
}

int uthread_delete(int tid)
{
    uthread_t *prev = NULL, *target = NULL;
    for (uthread_t *t = all_threads; t != NULL; t = t->all_next) {
        if (t->tid == tid) { target = t; break; }
        prev = t;
    }
    if (target == NULL)                           return -1;
    if (target->state == UTHREAD_RUNNING)         return -1;
    if (target->state == UTHREAD_BLOCKED)         return -1;
    /* 有等待者时禁止删除，防止 use-after-free */
    if (target->waiting_thread != NULL)           return -1;

    if (target->state == UTHREAD_READY)
        queue_remove(&ready_queue_head, &ready_queue_tail, tid);

    if (prev != NULL) prev->all_next = target->all_next;
    else              all_threads    = target->all_next;

    free(target->stack);
    free(target);
    return 0;
}

int uthread_set_priority(int tid, int priority)
{
    for (uthread_t *t = all_threads; t != NULL; t = t->all_next) {
        if (t->tid == tid) {
            t->priority = priority;
            return 0;
        }
    }
    return -1;
}

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
