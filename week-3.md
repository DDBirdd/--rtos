
简单RTOS任务调度框架调试

# 一、RTOS任务调度框架设计

该RTOS任务调度框架采用优先级抢占式调度算法，现代码主要包含以下几个部分：
任务状态定义：分为就绪、运行、阻塞和挂起四种状态。
任务控制块（TCB）：包含任务名称、函数指针、参数、状态、优先级、任务栈及链表指针。
系统控制块（SCB）：管理当前任务、就绪队列和滴答计数器。
任务队列操作：根据优先级排序任务，高优先级任务排在前面。
任务创建与删除：创建任务时分配内存并加入就绪队列，删除任务时移除并释放内存。
调度器：取出优先级最高的任务运行，中断时重新加入就绪队列。
时间滴答处理：模拟中断行为，实现时间片轮转调度。

---

# 二、RTOS任务调度框架测试

为了验证任务调度框架的正确性与功能完整，设计了一系列测试用例，涵盖任务创建、优先级调度、任务切换、任务停止等关键功能。以下是具体的测试过程与结果分析。

## （一）测试环境搭建
选择C语言作为开发工具，在个人计算机上运行测试程序：
通过手动调用调度器函数来模拟中断触发的行为，可以在简单的C编译器上完成调度管理子系统的设计、编写和调试，而无需的硬件环境支持。

## （二）测试任务函数
设计了task1和task2两个测试任务函数，模拟任务运行并打印信息观察执行情况。

### 1.任务函数实现
```c
void task1(void* arg) {
    int* count = (int*)arg;
    while (running) {
        printf("Task1 running, count = %d\n", (*count)++);
        // 模拟任务运行一段时间
        for (volatile int i = 0; i < 1000000; i++);
        // 让出CPU
        rtos_schedule();
    }
}

```
任务函数通过while循环模拟持续运行，打印任务名称和计数器值，加入延时循环模拟运行时间，并调用rtos_schedule()让出CPU。

### 2.任务函数设计考虑
- **任务运行时间模拟**：通过延时循环确保任务不会立即完成。
- **任务让出CPU**：调用rtos_schedule()模拟任务让出CPU行为。
- **任务停止条件**：加入全局变量running检查，满足条件时停止任务。

## （三）任务创建与调度测试
任务创建与调度是RTOS任务调度框架的核心功能之一。我们通过在主函数中创建任务并启动调度，验证任务是否能够正确地被创建、加入就绪队列，并按照优先级进行调度。

### 1.任务创建
使用rtos_task_create函数创建任务，为任务传递计数器变量作为参数，设置不同优先级。
```c
int count1 = 0;
Task* task1 = rtos_task_create("Task1", task1, &count1, 2);

```
同时，我们为`task1`设置了优先级2，为`task2`设置了优先级1。

### 2.任务调度
通过主函数中的while循环不断调用rtos_schedule实现调度。
```c
while (running) {
    rtos_schedule();
}
```
在调度过程中，调度器会从就绪队列中取出优先级最高的任务，将其设置为当前任务并运行。如果当前任务被中断，会将其状态设置为就绪并重新加入就绪队列。

### 3.测试结果观察
输出结果类似如下：
```
Task2 running, count = 0
Task2 running, count = 1
Task1 running, count = 0
Task2 running, count = 2
Task1 running, count = 1
...
```
任务按优先级顺序执行，优先级高的task2更多地出现在输出中，表明调度框架能按优先级调度。

## （三）任务停止机制测试
在主函数中创建任务并启动调度，验证任务创建、加入就绪队列及按优先级调度的功能。
### 1.任务创建
使用rtos_task_create函数创建任务，为任务传递计数器变量作为参数，设置不同优先级。
```c
if (*count >= 10) {
    rtos_stop(); // 满足条件后停止程序
    break;
}
```
### 2.任务停止函数实现
通过主函数中的while循环不断调用rtos_schedule实现调度。
```c
void rtos_stop() {
    running = false; // 设置退出标志
}
```
当`running`被设置为`false`时，主函数中的调度循环会退出，程序停止运行。
















