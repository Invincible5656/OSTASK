# 用户级线程库课程设计实施规划

## 1. 项目定位

本项目面向操作系统课程设计，目标是在 Linux 环境下使用 C 语言实现一个用户级线程库。线程库需要在用户态完成线程创建、线程让步、线程退出、线程调度、线程删除、线程信息查看和优先级设置。

主线方案：

```text
C 语言
+ Linux ucontext
+ 用户态线程控制块 TCB
+ ready / running / blocked 状态管理
+ FIFO / RR / Priority 三种调度算法
+ 测试程序和课程报告
```

开发方式为：

```text
Mac + VSCode：写代码、整理文档、远程连接服务器
腾讯云 Linux：编译、运行、调试、截图、最终验收
```

注意：本项目最终运行环境应统一放在腾讯云 Linux 上。

## 2. 必须完成的功能

线程库至少支持：

- 创建线程，并保存线程信息
- 主动让出 CPU，即 `yield`
- 线程退出，即 `exit`
- 线程调度
- 线程删除
- 查看线程池中所有线程信息
- 手动设置线程优先级
- 支持 FIFO、RR、优先级调度

每个线程至少保存：

- 线程 ID
- 线程优先级
- 所属进程 ID
- 创建时间
- 线程状态

线程状态至少展示：

- `ready`
- `running`
- `blocked`

实现中可以额外维护 `exited` 状态，方便处理线程退出和资源回收。

## 3. 工程结构

项目目录命名为 `uthread-project`：

```text
uthread-project/
├── include/
│   └── uthread.h
├── src/
│   ├── uthread.c
│   ├── scheduler.c
│   ├── queue.c
│   └── timer.c
├── tests/
│   ├── test_basic.c
│   ├── test_fifo.c
│   ├── test_rr.c
│   ├── test_priority.c
│   ├── test_join.c
│   └── test_stress.c
├── docs/
│   ├── design.md
│   ├── api.md
│   └── report.md
├── Makefile
└── README.md
```

各文件职责：

| 文件 | 作用 |
|---|---|
| `include/uthread.h` | 对外 API、线程状态、调度策略、常量定义 |
| `src/uthread.c` | 线程创建、退出、让步、等待、删除、信息查看 |
| `src/scheduler.c` | FIFO、RR、Priority 调度逻辑 |
| `src/queue.c` | 线程队列入队、出队、查找、删除 |
| `src/timer.c` | 可选：`setitimer` 抢占式 RR |
| `tests/` | 各功能测试程序 |
| `docs/` | 设计文档、API 文档、报告草稿 |
| `Makefile` | 编译、运行、清理 |

## 4. 核心数据结构设计

线程控制块 TCB 是本项目核心。设计为：

```c
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

typedef struct uthread {
    int tid;
    int priority;
    pid_t pid;
    time_t create_time;
    uthread_state_t state;
    ucontext_t context;
    void *stack;
    void *retval;
    struct uthread *waiting_thread;
    struct uthread *next;
} uthread_t;
```

关键字段说明：

| 字段 | 说明 |
|---|---|
| `tid` | 用户级线程 ID，由线程库分配 |
| `priority` | 线程优先级，数字越小优先级越高 |
| `pid` | 所属进程 ID，使用 `getpid()` 获取 |
| `create_time` | 创建时间 |
| `state` | 当前状态 |
| `context` | `ucontext_t` 上下文 |
| `stack` | 用户级线程私有栈 |
| `retval` | 线程退出返回值 |
| `waiting_thread` | 等待该线程结束的线程 |
| `next` | 队列链表指针 |

线程库全局管理结构建议维护：

```text
current_thread      当前运行线程
ready_queue         就绪队列
all_threads         全部线程表或链表
sched_policy        当前调度算法
next_tid            下一个线程 ID
```

## 5. 对外 API 规划

头文件 `include/uthread.h` 暴露以下接口：

```c
int uthread_init(uthread_sched_policy_t policy);

int uthread_create(
    int *tid,
    void *(*start_routine)(void *),
    void *arg,
    int priority
);

void uthread_yield(void);

void uthread_exit(void *retval);

int uthread_join(int tid, void **retval);

int uthread_delete(int tid);

int uthread_set_priority(int tid, int priority);

void uthread_list(void);

void uthread_start(void);
```

接口实现重点：

| API | 重点 |
|---|---|
| `uthread_init` | 初始化主线程、队列、调度策略 |
| `uthread_create` | 分配 TCB 和栈，调用 `getcontext`、`makecontext` |
| `uthread_yield` | 当前线程主动让出 CPU，触发调度 |
| `uthread_exit` | 设置退出状态，保存返回值，唤醒等待线程 |
| `uthread_join` | 当前线程进入 blocked，等待目标线程结束 |
| `uthread_delete` | 删除 ready 或 exited 线程，释放资源 |
| `uthread_set_priority` | 修改线程优先级 |
| `uthread_list` | 打印线程 ID、PID、优先级、状态、创建时间 |
| `uthread_start` | 启动调度器 |

## 6. 调度算法设计

### 6.1 FIFO 调度

FIFO 是先进先出调度，适合先实现。

规则：

- 新线程加入 ready 队列尾部
- 调度器从 ready 队列头部取线程运行
- 线程运行到主动 `yield` 或 `exit`
- 已退出线程不再进入 ready 队列

特点：

- 实现简单
- 非抢占式
- 如果线程不主动让出 CPU，后续线程无法运行

### 6.2 RR 调度

RR 是轮转调度。课程项目建议先实现协作式 RR。

协作式 RR 规则：

- 每个线程执行一小段后调用 `uthread_yield`
- 当前线程如果未退出，重新放回 ready 队列尾部
- 调度器从队首取下一个线程

示例顺序：

```text
T1 step 1
T2 step 1
T3 step 1
T1 step 2
T2 step 2
T3 step 2
```

可选增强：

- 使用 `setitimer`
- 使用 `SIGALRM`
- 定时触发调度，实现抢占式 RR

### 6.3 优先级调度

优先级规则建议为：

```text
priority 数字越小，优先级越高
```

实现方式：

- ready 线程统一放在一个队列中
- 每次调度扫描 ready 队列
- 选择 priority 最小的线程
- 相同优先级保持 FIFO 顺序

示例：

```text
T1 priority = 5
T2 priority = 1
T3 priority = 3

运行顺序：T2 -> T3 -> T1
```

优先级调度可能导致低优先级线程饥饿，后续可通过老化机制改进。

## 7. 分阶段实施步骤

### 阶段一：搭建工程骨架

目标：

- 建立目录结构
- 写好 `Makefile`
- 保证空项目可以编译

相关命令：

```bash
mkdir -p include src tests docs
touch include/uthread.h
touch src/uthread.c src/scheduler.c src/queue.c src/timer.c
touch tests/test_basic.c tests/test_fifo.c tests/test_rr.c
touch tests/test_priority.c tests/test_join.c tests/test_stress.c
touch docs/design.md docs/api.md docs/report.md
touch Makefile
```

### 阶段二：实现队列和 TCB

目标：

- 定义线程状态枚举
- 定义调度算法枚举
- 定义 TCB
- 实现 ready queue 的入队、出队、删除、查找


### 阶段三：实现线程初始化和创建

目标：

- `uthread_init`
- `uthread_create`
- 为每个线程分配独立栈
- 使用 `getcontext` 和 `makecontext` 创建可运行上下文

注意：

- 栈大小建议 `64KB` 或 `128KB`
- 线程函数返回后必须进入 `uthread_exit`
- 主线程也要被纳入线程管理

### 阶段四：实现 yield 和基本调度

目标：

- `uthread_yield`
- 调度器选择下一个 ready 线程
- 使用 `swapcontext` 完成上下文切换


### 阶段五：实现 FIFO 和 RR

目标：

- FIFO：验证创建顺序和运行顺序
- RR：验证多个线程轮流运行

测试：

- 当前线程 yield 后是否重新进入队尾
- 退出线程是否不会再次被调度
- ready 队列是否维护正确

### 阶段六：实现优先级调度

目标：

- `uthread_set_priority`
- 调度时选取最高优先级线程
- 相同优先级按 FIFO 顺序执行

测试：

- 创建顺序和优先级顺序不一致时，是否按优先级运行
- 运行过程中修改优先级是否生效

### 阶段七：实现 join、delete、list

目标：

- `uthread_join`
- `uthread_delete`
- `uthread_list`

重点：

- join 时当前线程进入 `blocked`
- 目标线程退出后唤醒等待线程
- 当前运行线程不能直接删除自身
- 删除线程时要从队列和线程表中移除
- 线程栈不能在当前线程仍运行在该栈上时释放

### 阶段八：完善测试和文档

目标：

- 所有测试程序可运行
- README、设计文档、API 文档、课程报告草稿完整
- 在腾讯云 Linux 上完整验证

最终执行：

```bash
make clean
make
make test
valgrind --leak-check=full ./test_stress
```

## 8. 测试程序规划

| 测试文件 | 测试目标 |
|---|---|
| `tests/test_basic.c` | 验证初始化、创建、yield、exit |
| `tests/test_fifo.c` | 验证 FIFO 顺序 |
| `tests/test_rr.c` | 验证 RR 轮转 |
| `tests/test_priority.c` | 验证优先级调度 |
| `tests/test_join.c` | 验证 blocked、join、唤醒 |
| `tests/test_stress.c` | 批量创建线程，检查稳定性 |

基础测试期望：

```text
main start
thread 1 running
thread 2 running
thread 1 exit
thread 2 exit
main end
```

RR 测试期望：

```text
T1 step 1
T2 step 1
T3 step 1
T1 step 2
T2 step 2
T3 step 2
```

优先级测试期望：

```text
High priority thread running
Middle priority thread running
Low priority thread running
```

## 9. Makefile 目标建议

支持：

```text
make                编译所有测试程序
make test           运行所有测试
make test_basic     运行基础测试
make test_fifo      运行 FIFO 测试
make test_rr        运行 RR 测试
make test_priority  运行优先级测试
make test_join      运行 join 测试
make test_stress    运行压力测试
make clean          清理构建文件
```

手动编译示例：

```bash
gcc -Wall -Wextra -g -Iinclude \
    src/uthread.c src/scheduler.c src/queue.c \
    tests/test_basic.c \
    -o test_basic
```

## 10. 腾讯云 Linux 部署流程

### 10.1 上传项目

使用 `scp`：

```bash
scp -r uthread-project user@server_ip:/home/user/
```

或使用 Git：

```bash
git clone <your-repository-url>
cd uthread-project
```

### 10.2 安装依赖

Ubuntu / Debian：

```bash
sudo apt update
sudo apt install build-essential gdb valgrind
```

CentOS / Rocky Linux：

```bash
sudo yum groupinstall "Development Tools"
sudo yum install gdb valgrind
```

### 10.3 编译运行

```bash
cd uthread-project
make clean
make
make test
```

### 10.4 调试

```bash
gdb ./test_basic
```

常用 GDB 命令：

```text
break main
break uthread_create
break uthread_yield
break uthread_exit
run
next
step
continue
bt
quit
```

### 10.5 内存检查

```bash
valgrind --leak-check=full ./test_basic
valgrind --leak-check=full ./test_stress
```