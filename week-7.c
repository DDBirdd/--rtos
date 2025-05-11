/*
最后修改于2025.5.11
北京理工大学计算机软件基础项目 rtos系统
week-6，将之前做的单独的模块试着整合到了一起，这里面写的之前包含了linux特有的一些代码，如果要在windows上运行的话还需要做一些系统适配
目前已基本完成的有任务系统与抢断系统，未来预计以头函数的形式管理，在那时会上传新的代码文件
*/



#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>

// 任务状态定义
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_SUSPENDED
} TaskState;

// 任务控制块（TCB）
typedef struct _Task {
    char* name;
    void (*func)(void*);
    void* arg;
    TaskState state;
    uint32_t priority;
    uint32_t stack[1024]; // 任务栈
    struct _Task* next;
} Task;

// 系统控制块（SCB）
typedef struct {
    Task* current_task;
    Task* ready_list;
    uint32_t tick;
} RTOS;

// 全局系统控制块实例
RTOS rtos = {0};

// 任务队列操作函数
static void task_enqueue(Task** head, Task* task) {
    Task* current = *head;
    if (!current) {
        *head = task;
        return;
    }
    
    // 简单优先级排序（高优先级在前）
    while (current->next && current->next->priority >= task->priority) {
        current = current->next;
    }
    task->next = current->next;
    current->next = task;
}

static Task* task_dequeue(Task** head) {
    Task* task = *head;
    if (task) {
        *head = task->next;
        task->next = NULL;
    }
    return task;
}

// 内存管理（简化实现）
static void* rtos_malloc(size_t size) {
    return malloc(size);
}

static void rtos_free(void* ptr) {
    free(ptr);
}

// 任务创建函数
Task* rtos_task_create(char* name, void (*func)(void*), void* arg, uint32_t priority) {
    Task* task = rtos_malloc(sizeof(Task));
    memset(task, 0, sizeof(Task));
    task->name = name;
    task->func = func;
    task->arg = arg;
    task->priority = priority;
    task->state = TASK_READY;
    task_enqueue(&rtos.ready_list, task);
    return task;
}

// 任务删除函数
void rtos_task_delete(Task* task) {
    if (!task) return;
    
    // 从就绪队列中移除
    Task** current = &rtos.ready_list;
    while (*current) {
        if (*current == task) {
            *current = task->next;
            break;
        }
        current = &(*current)->next;
    }
    
    // 释放内存
    rtos_free(task);
}

// 调度器（简单抢占式）
void rtos_schedule() {
    Task* next_task = task_dequeue(&rtos.ready_list);
    if (!next_task) return;
    
    if (rtos.current_task) {
        // 保存当前任务上下文
        // 实际实现需要汇编代码保存寄存器
        // 此处简化处理
        rtos.current_task->state = TASK_READY;
        task_enqueue(&rtos.ready_list, rtos.current_task);
    }
    
    next_task->state = TASK_RUNNING;
    rtos.current_task = next_task;
    next_task->func(next_task->arg); // 直接运行任务函数
}

// 系统初始化
void rtos_init() {
    rtos.tick = 0;
    rtos.ready_list = NULL;
    rtos.current_task = NULL;
}

// 时间滴答处理（需硬件定时器触发）
void rtos_tick_handler() {
    rtos.tick++;
    // 简单时间片轮转调度
    if (rtos.current_task) {
        rtos.current_task->state = TASK_READY;
        task_enqueue(&rtos.ready_list, rtos.current_task);
        rtos_schedule();
    }
}

// 示例任务函数
void task1(void* arg) {
    int* count = (int*)arg;
    volatile bool running = true; // 全局变量，控制程序是否继续运行
    if (running) {
        printf("Task1 running, count = %d\n", (*count)++);
        // 模拟任务运行一段时间
        for (volatile int i = 0; i < 1000000; i++);
        // 让出CPU
        if (*count==10) {
            running = false; // 设置退出标志
        }
        rtos_schedule();
    }
}

void task2(void* arg) {
    int* count = (int*)arg;
    volatile bool running = true; // 全局变量，控制程序是否继续运行
    if (running) {
        printf("Task2 running, count = %d\n", (*count)++);
        // 模拟任务运行一段时间
        for (volatile int i = 0; i < 1000000; i++);
        // 让出CPU
        rtos_schedule();
    }
}

void task3(void* arg) {
    int* count = (int*)arg;
    volatile bool running = true; // 全局变量，控制程序是否继续运行
    if (running) {
        printf("Task3 running, count = %d\n", (*count)++);
        // 模拟任务运行一段时间
        for (volatile int i = 0; i < 1000000; i++);
        // 让出CPU
        rtos_schedule();
    }
}

// 用户态-内核态共享环形队列（Ring Buffer）
typedef struct {
    volatile uint32_t head;  // 用户态生产，写入，内核态在此位置读取，消费
    volatile uint32_t tail;  // 内核态生产，用户态消费
    uint32_t mask;           // 环形队列大小掩码（2的幂次），用于计算索引
    // TaskInterruptReq requests[];  // 这里需要定义 TaskInterruptReq 结构体
} UserInterruptRing;

typedef struct {
    volatile uint32_t head;  // 内核态生产，用户态消费
    volatile uint32_t tail;  // 用户态生产，内核态消费
    uint32_t mask;
    // TaskInterruptResp responses[]; // 这里需要定义 TaskInterruptResp 结构体
} KernelInterruptRing;

// 用户态注册中断处理函数
// 假设已经定义了 TaskInterruptReq 结构体
// 这里需要补全 TaskInterruptReq 结构体定义
typedef struct {
    int type;
    void (*handler_ptr)(void*);
    void* user_ctx;
} TaskInterruptReq;

void RegisterInterruptHandler(int irq, void (*handler)(void*), void* ctx) {
    UserInterruptRing* ring = (UserInterruptRing*)malloc(sizeof(UserInterruptRing)); // 这里简化，实际需要正确获取
    ring->requests[ring->head & ring->mask] = (TaskInterruptReq){
        .type = 1, // 假设 REGISTER_HANDLER 为 1
        .handler_ptr = handler,
        .user_ctx = ctx
    };
    // atomic_store(&ring->head, ring->head + 1); // 这里需要引入原子操作库
    // syscall_notify_kernel(); // 这里需要实现通知内核的系统调用
}

// 内核中断处理线程
// 假设已经定义了 TaskInterruptResp 结构体
// 这里需要补全 TaskInterruptResp 结构体定义
typedef struct {
    // 响应结构体内容
} TaskInterruptResp;

void KernelInterruptWorker() {
    while (1) {
        UserInterruptRing* req_ring = (UserInterruptRing*)malloc(sizeof(UserInterruptRing)); // 这里简化，实际需要正确获取
        KernelInterruptRing* resp_ring = (KernelInterruptRing*)malloc(sizeof(KernelInterruptRing)); // 这里简化，实际需要正确获取
        
        // 批量拉取请求
        uint32_t tail = req_ring->tail;
        uint32_t head = req_ring->head;
        for (int i = 0; i < (head - tail); i++) {
            TaskInterruptReq req = req_ring->requests[tail & req_ring->mask];
            if (req.type == 2) { // 假设 IO_REQUEST 为 2
                // SubmitAsyncIO(req); // 这里需要实现异步 IO 提交
            }
            tail++;
        }
        req_ring->tail = tail;

        // 检查完成事件并填充响应队列
        // CheckIOCompletion(resp_ring); // 这里需要实现检查 IO 完成事件
    }
}

// EDF/RR调度器增强
typedef struct {
    void (*run)(void);
    int priority;
    int period;
    int deadline;
    int timeSlice;
    int remainingTimeSlice;
    int state;
    int remainingTime;
} Task_t;

#define MAX_TASKS 10
static Task_t TaskList[MAX_TASKS];
static int TaskCount = 0;
static int CurrentTaskIndex = 0;

void Scheduler_Init(void) {
    TaskCount = 0;
}

void Scheduler_Start(void) {
    while (1) {
        // 优先检查用户态中断完成事件
        // ProcessUserInterrupts(); // 这里需要实现处理用户态中断完成事件

        Task_t* next_task = NULL;
        // if (HasPendingInterrupts()) {
        //     next_task = FindInterruptHandlerTask(); // 高优先级处理中断
        // } else {
        //     next_task = EDF_RR_SelectTask(); // 常规调度
        // }

        // ExecuteTaskWithTimeSlice(next_task); // 这里需要实现带时间片的任务执行
    }
}

// 带时间片的任务执行
void ExecuteTaskWithTimeSlice(Task_t* task) {
    int time_used = 0;
    while (time_used < task->remainingTimeSlice) {
        // if (CheckInterruptFlag()) { // 硬件中断检查
        //     SaveTaskContext(task);
        //     HandleHardwareInterrupt(); // 快速路径处理
        //     if (UserInterruptPending()) {
        //         break; // 立即切换到用户态处理程序
        //     }
        // }
        task->run();
        time_used++;
    }
    task->remainingTimeSlice -= time_used;
}

// 将中断线程绑定到专属CPU核心
void BindInterruptToCore(int core_id) {
    cpu_set_t cpuset;//这个是linux特有的系统标识符，用windows管理的话需要换为位掩码操作
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t interrupt_thread; // 这里需要正确获取中断线程
    pthread_setaffinity_np(interrupt_thread, sizeof(cpuset), &cpuset);
}

// 根据系统负载动态调整时间片
void AdjustTimeSlices() {
    float load = 0.5; // 这里简化，实际需要获取系统负载
    for (int i = 0; i < TaskCount; i++) {
        TaskList[i].timeSlice = 10 * (1.0f + load); // 假设 BASE_TIME_SLICE 为 10
    }
}

int main() {
    rtos_init();
    
    // 创建任务1
    int count1 = 0;
    rtos_task_create("Task1", task1, &count1, 1);

    // 创建任务2
    int count2 = 0;
    rtos_task_create("Task2", task2, &count2, 3);

    int count3 = 0;
    rtos_task_create("Task3", task3, &count3, 2);

    volatile bool running = true;
    while (running) { // 使用全局变量控制循环
        rtos_schedule();
    }

    return 0;
}
