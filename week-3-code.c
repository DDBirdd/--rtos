#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


volatile bool running = true; // 全局变量，控制程序是否继续运行

void rtos_stop() {
    running = false; // 设置退出标志
}

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
    if (running) {
        printf("Task1 running, count = %d\n", (*count)++);
        // 模拟任务运行一段时间
        for (volatile int i = 0; i < 1000000; i++);
        // 让出CPU
        if (*count==10) rtos_stop();
        rtos_schedule();
    }
}

void task2(void* arg) {
    int* count = (int*)arg;
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
    if (running) {
        printf("Task3 running, count = %d\n", (*count)++);
        // 模拟任务运行一段时间
        for (volatile int i = 0; i < 1000000; i++);
        // 让出CPU
        rtos_schedule();
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
 	 
       while (running) { // 使用全局变量控制循环
        rtos_schedule();
    } 


    
	//while (1) {
     //   rtos_tick_handler(); // 模拟时间滴答中断
    //}
    
    // 启动调度
    //while (1) {
    //    rtos_schedule();
    //}
    
    return 0;
}
