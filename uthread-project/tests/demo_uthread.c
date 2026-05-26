#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uthread.h"    

/* TID 跨线程传递用全局变量（与 test_join.c 同一惯例） */
static int g_worker_tid;
static int g_t2_tid, g_t3_tid;

/* ============================================================
 * Demo 1: FIFO 调度
 * ============================================================ */

static void *fifo_thread(void *arg)
{
    printf("  T%d 运行\n", (int)(intptr_t)arg);
    return NULL;
}

static void demo_fifo(void)
{
    printf("\n[Demo 1] FIFO 调度演示\n");
    printf("创建顺序: T1 -> T2 -> T3 (优先级相同)\n");
    printf("期望执行顺序: T1 -> T2 -> T3\n\n");

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, fifo_thread, (void *)1, 0);
    uthread_create(NULL, fifo_thread, (void *)2, 0);
    uthread_create(NULL, fifo_thread, (void *)3, 0);
    uthread_start();

    printf("\n结论: FIFO 按就绪队列进入顺序依次执行\n");
}

/* ============================================================
 * Demo 2: FIFO vs 协作式 RR 对比
 * ============================================================ */

static void *fifo_cmp_thread(void *arg)
{
    int id = (int)(intptr_t)arg;
    for (int i = 1; i <= 3; i++)
        printf("  T%d-step%d\n", id, i);
    return NULL;
}

static void *rr_cmp_thread(void *arg)
{
    int id = (int)(intptr_t)arg;
    for (int i = 1; i <= 3; i++) {
        printf("  T%d-step%d\n", id, i);
        if (i < 3) uthread_yield();
    }
    return NULL;
}

static void demo_rr(void)
{
    printf("\n[Demo 2] RR 调度演示（含 FIFO 对比）\n");

    printf("\n--- FIFO 模式：线程不主动让出，运行至完成再切换 ---\n");
    printf("期望: T1 全部步骤 -> T2 全部步骤 -> T3 全部步骤\n\n");
    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL, fifo_cmp_thread, (void *)1, 0);
    uthread_create(NULL, fifo_cmp_thread, (void *)2, 0);
    uthread_create(NULL, fifo_cmp_thread, (void *)3, 0);
    uthread_start();

    printf("\n--- 协作式 RR 模式：线程每步后主动 yield ---\n");
    printf("期望: T1-s1 -> T2-s1 -> T3-s1 -> T1-s2 -> T2-s2 -> T3-s2 -> ...\n\n");
    uthread_init(UTHREAD_SCHED_RR);
    uthread_create(NULL, rr_cmp_thread, (void *)1, 0);
    uthread_create(NULL, rr_cmp_thread, (void *)2, 0);
    uthread_create(NULL, rr_cmp_thread, (void *)3, 0);
    uthread_start();

    printf("\n结论: 协作式 RR 依赖线程主动 yield 实现时间片轮转，无抢占\n");
}

/* ============================================================
 * Demo 3: 优先级调度
 * ============================================================ */

static void *prio_thread(void *arg)
{
    int id = (int)(intptr_t)arg;
    printf("  T%d (priority=%d) 运行\n", id, current_thread->priority);
    return NULL;
}

static void demo_priority(void)
{
    printf("\n[Demo 3] 优先级调度演示\n");
    printf("创建: T1(p=5)  T2(p=1)  T3(p=3)\n");
    printf("期望执行顺序（数值越小优先级越高）: T2 -> T3 -> T1\n\n");

    uthread_init(UTHREAD_SCHED_PRIORITY);
    uthread_create(NULL, prio_thread, (void *)1, 5);
    uthread_create(NULL, prio_thread, (void *)2, 1);
    uthread_create(NULL, prio_thread, (void *)3, 3);
    uthread_start();

    printf("\n结论: 优先级调度每轮从就绪队列选 priority 值最小的线程\n");
}

/* ============================================================
 * Demo 4: 线程信息查看与管理
 *
 * 使用优先级调度：管理线程(p=1)最先运行，在其内部调用
 * uthread_list 展示动态快照，然后修改 T3 优先级并删除 T2，
 * 最终调度结果为 T3(p=1) -> T1(p=5)。
 *
 * 注：TID 0 主线程状态显示为 RUNNING 是库的设计特征——
 * uthread_start 保存上下文但未将主线程入队或改变其状态。
 * ============================================================ */

static void *mgr_thread(void *arg)
{
    (void)arg;
    printf("  [管理线程] 运行中，列出当前所有线程快照:\n");
    printf("  (TID 0 为主线程上下文，state=RUNNING 为库内部设计)\n\n");
    uthread_list();

    printf("\n  [管理线程] 修改 T3 优先级: 8 -> 1\n");
    uthread_set_priority(g_t3_tid, 1);

    printf("  [管理线程] 删除 T2 (TID=%d): %s\n",
           g_t2_tid,
           uthread_delete(g_t2_tid) == 0 ? "成功" : "失败");

    printf("  [管理线程] 管理完成，退出\n\n");
    return NULL;
}

static void *work_thread(void *arg)
{
    int id = (int)(intptr_t)arg;
    printf("  T%d 被调度执行 (priority=%d)\n", id, current_thread->priority);
    return NULL;
}

static void demo_manage(void)
{
    printf("\n[Demo 4] 线程信息查看与管理演示\n");
    printf("创建: T1(p=5)  T2(p=3)  T3(p=8)  管理线程(p=1)\n");
    printf("管理线程最先运行 -> uthread_list -> 改 T3 优先级 -> 删除 T2\n\n");

    uthread_init(UTHREAD_SCHED_PRIORITY);
    int t1_tid;
    uthread_create(&t1_tid,   work_thread, (void *)1, 5);
    uthread_create(&g_t2_tid, work_thread, (void *)2, 3);
    uthread_create(&g_t3_tid, work_thread, (void *)3, 8);
    uthread_create(NULL,      mgr_thread,  NULL,      1);
    uthread_start();

    printf("\n结论: 调度运行中可动态查看、修改优先级、删除 READY 线程\n");
}

/* ============================================================
 * Demo 5: join 阻塞与唤醒
 *
 * Joiner 先入队（FIFO 先执行），调用 join 时 Worker 尚未退出
 * 故阻塞；Worker 被调度后退出，唤醒 Joiner 并传递返回值。
 * g_worker_tid 在 uthread_start 前由主线程设置，线程函数安全读取。
 * ============================================================ */

static void *join_worker(void *arg)
{
    (void)arg;
    printf("  Worker 开始运行，计算结果...\n");
    uthread_exit((void *)42);
    return NULL;
}

static void *join_joiner(void *arg)
{
    (void)arg;
    printf("  Joiner: 调用 join 等待 Worker (TID=%d)...\n", g_worker_tid);
    void *retval = NULL;
    uthread_join(g_worker_tid, &retval);
    printf("  Joiner: 被唤醒，获取返回值 %ld\n", (long)retval);
    return NULL;
}

static void demo_join(void)
{
    printf("\n[Demo 5] join 阻塞与唤醒演示\n");
    printf("Joiner 先创建(队列位置1) -> 先运行 -> join 阻塞\n");
    printf("Worker 被调度 -> 退出 -> 唤醒 Joiner -> Joiner 取返回值\n\n");

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(NULL,          join_joiner, NULL, 0); /* 先入队 */
    uthread_create(&g_worker_tid, join_worker, NULL, 0); /* TID 在 start 前已设置 */
    uthread_start();

    printf("\n结论: join 阻塞调用线程直到目标退出，返回值通过指针参数传递\n");
}

/* ============================================================
 * Demo 6: delete 安全删除
 * ============================================================ */

static void *del_t1_func(void *arg)
{
    (void)arg;
    printf("  T1: 不应出现（已被删除）\n");
    return NULL;
}

static void *del_t2_func(void *arg)
{
    (void)arg;
    printf("  T2: 正常执行\n");
    return NULL;
}

static void demo_delete(void)
{
    int t1_tid;
    printf("\n[Demo 6] delete 安全删除演示\n");
    printf("创建 T1、T2，在 uthread_start 前删除 T1\n");
    printf("期望: 只有 T2 被执行\n\n");

    uthread_init(UTHREAD_SCHED_FIFO);
    uthread_create(&t1_tid, del_t1_func, NULL, 0);
    uthread_create(NULL,    del_t2_func, NULL, 0);

    int rc = uthread_delete(t1_tid);
    printf("  [主线程] 删除 T1 (TID=%d): %s\n",
           t1_tid, rc == 0 ? "成功" : "失败");
    printf("  启动调度:\n");
    uthread_start();

    printf("\n结论: delete 将 READY 线程从就绪队列移除，调度后不再执行\n");
}

/* ============================================================
 * 菜单主循环
 * ============================================================ */

static void print_menu(void)
{
    printf("\n====================================\n");
    printf("  用户级线程库实验演示系统\n");
    printf("====================================\n");
    printf("  1. FIFO 调度演示\n");
    printf("  2. RR 调度演示（含 FIFO 对比）\n");
    printf("  3. 优先级调度演示\n");
    printf("  4. 线程信息查看与管理\n");
    printf("  5. join 阻塞与唤醒演示\n");
    printf("  6. delete 安全删除演示\n");
    printf("  0. 退出\n");
    printf("====================================\n");
    printf("请选择: ");
    fflush(stdout);
}

int main(void)
{
    int choice;
    do {
        print_menu();
        if (scanf("%d", &choice) != 1) break;
        switch (choice) {
            case 1: demo_fifo();     break;
            case 2: demo_rr();       break;
            case 3: demo_priority(); break;
            case 4: demo_manage();   break;
            case 5: demo_join();     break;
            case 6: demo_delete();   break;
            case 0: printf("再见！\n"); break;
            default: printf("无效选项，请重新输入\n"); break;
        }
    } while (choice != 0);

    return 0;
}
