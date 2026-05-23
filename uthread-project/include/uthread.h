#ifndef UTHREAD_H
#define UTHREAD_H

#include <ucontext.h>
#include <time.h>
#include <sys/types.h>

#define UTHREAD_STACK_SIZE   (64 * 1024)
#define UTHREAD_MAX_THREADS  64

/* 线程状态 */
typedef enum {
    UTHREAD_READY,
    UTHREAD_RUNNING,
    UTHREAD_BLOCKED,
    UTHREAD_EXITED
} uthread_state_t;

/* 调度策略：FIFO / 时间片轮转 / 优先级 */
typedef enum {
    UTHREAD_SCHED_FIFO,
    UTHREAD_SCHED_RR,
    UTHREAD_SCHED_PRIORITY
} uthread_sched_policy_t;

/* 线程控制块 TCB */
typedef struct uthread {
    int                  tid;            /* 线程 ID */
    int                  priority;       /* 数值越小优先级越高 */
    pid_t                pid;            /* 所属进程 ID */
    time_t               create_time;    /* 创建时间 */
    uthread_state_t      state;
    ucontext_t           context;
    void                *stack;          /* 私有栈 */
    void                *(*func)(void *);
    void                *arg;
    void                *retval;         /* 退出返回值 */
    struct uthread      *waiting_thread; /* 在本线程上 join 的等待者 */
    struct uthread      *next;           /* 就绪队列链 */
    struct uthread      *all_next;       /* 全部线程链 */
} uthread_t;

/* 跨模块共享的全局变量 */
extern uthread_t            *current_thread;
extern uthread_t            *all_threads;
extern uthread_sched_policy_t sched_policy;
extern int                   next_tid;
extern uthread_t            *ready_queue_head;
extern uthread_t            *ready_queue_tail;

/* 库初始化与启动 */
int  uthread_init(uthread_sched_policy_t policy);
void uthread_start(void);

/* 线程管理 */
int  uthread_create(int *tid, void *(*start_routine)(void *),
                    void *arg, int priority);
void uthread_yield(void);
void uthread_exit(void *retval);
int  uthread_join(int tid, void **retval);
int  uthread_delete(int tid);

/* 信息查询与配置 */
int  uthread_set_priority(int tid, int priority);
void uthread_list(void);

/* 调度器：选择下一个线程并切换上下文 */
void schedule(void);

/* 就绪队列操作（基于 uthread_t.next 的侵入式单链表） */
void       queue_enqueue(uthread_t **head, uthread_t **tail, uthread_t *t);
uthread_t *queue_dequeue(uthread_t **head, uthread_t **tail);
uthread_t *queue_find(uthread_t *head, int tid);
uthread_t *queue_remove(uthread_t **head, uthread_t **tail, int tid);

#endif
