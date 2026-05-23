#include "uthread.h"

#include <stddef.h>

uthread_t *ready_queue_head = NULL;
uthread_t *ready_queue_tail = NULL;

/* 队列耗尽时由 schedule() 置 1，uthread_start() 据此返回主线程 */
int sched_done = 0;

/* 选择下一个线程并切换上下文：
   FIFO/RR 取队首；优先级则扫描整个队列选 priority 最小者；
   队列为空时切回主线程上下文 */
void schedule(void)
{
    uthread_t *next = NULL;

    if (sched_policy == UTHREAD_SCHED_PRIORITY) {
        uthread_t *best = NULL;
        for (uthread_t *cur = ready_queue_head; cur != NULL; cur = cur->next) {
            if (best == NULL || cur->priority < best->priority)
                best = cur;
        }
        if (best != NULL)
            next = queue_remove(&ready_queue_head, &ready_queue_tail, best->tid);
    } else {
        next = queue_dequeue(&ready_queue_head, &ready_queue_tail);
    }

    if (next == NULL) {
        sched_done = 1;
        uthread_t *prev = current_thread;
        current_thread     = all_threads;
        all_threads->state = UTHREAD_RUNNING;
        swapcontext(&prev->context, &all_threads->context);
        return;
    }

    uthread_t *prev = current_thread;
    next->state    = UTHREAD_RUNNING;
    current_thread = next;
    swapcontext(&prev->context, &next->context);
}
