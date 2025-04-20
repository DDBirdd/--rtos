为了实现低时延、高确定性的任务调度，并支持用户态中断等高级特性，我们尝试结合了最早截止时间优先和时间片轮转算法。这些算法通常用于实时系统中，以减少调度开销并提供更好的任务响应时间。

### 1. 任务结构体扩展
首先，我们需要扩展任务结构体以包含任务的截止时间和周期信息。
同时为支持最早截止时间优先和时间片轮转调度策略，我们可以在任务结构体中增加一个时间片字段，并在调度逻辑中结合这两种策略。

```c
typedef struct {
    TaskFunction_t run;          // 任务函数指针
    int priority;                // 任务优先级
    int period;                  // 任务周期
    int deadline;                // 任务截止时间
    int timeSlice;               // 时间片长度
    int remainingTimeSlice;      // 剩余时间片
    int state;                   // 任务状态
    int remainingTime;           // 剩余执行时间
} Task_t;
``` 
 
### 2. 任务创建接口
在创建任务时，需要指定任务的周期、截止时间和时间片。

```c
void Task_Create(TaskFunction_t taskFunction, int period, int deadline, int timeSlice) {
    if (TaskCount < MAX_TASKS) {
        Task_t newTask;
        newTask.run = taskFunction;
        newTask.period = period;
        newTask.deadline = deadline;
        newTask.timeSlice = timeSlice;
        newTask.remainingTimeSlice = timeSlice;
        newTask.priority = CalculatePriority(newTask.period); // 根据周期计算优先级
        newTask.state = TASK_READY;
        newTask.remainingTime = period; // 初始剩余时间为一个周期

        // 插入新任务并保持任务列表按优先级排序
        int i;
        for (i = TaskCount - 1; i >= 0 && TaskList[i].priority < newTask.priority; i--) {
            TaskList[i + 1] = TaskList[i];
        }
        TaskList[i + 1] = newTask;
        TaskCount++;
    }
}
```

### 3. 任务调度逻辑
在调度器中，结合EDF和RR策略来选择下一个任务。

```c
void Scheduler_Start(void) {
    while (1) {
        Task_t* nextTask = NULL;
        int nextDeadline = INT_MAX;

        // 找到最早截止时间的任务
        for (int i = 0; i < TaskCount; i++) {
            if (TaskList[i].state == TASK_READY && TaskList[i].deadline < nextDeadline) {
                nextDeadline = TaskList[i].deadline;
                nextTask = &TaskList[i];
            }
        }

        if (nextTask != NULL) {
            if (nextTask->remainingTime > 0) {
                nextTask->run();
                nextTask->remainingTime--;

                // 如果时间片用完，则进行任务切换
                if (nextTask->remainingTimeSlice <= 0) {
                    Scheduler_Yield(nextTask);
                    nextTask->remainingTimeSlice = nextTask->timeSlice;
                }
            } else {
                // 任务完成一个周期，更新其状态和剩余时间
                nextTask->remainingTime = nextTask->period;
                nextTask->state = TASK_READY;
            }

            // 检查是否需要调整任务状态
            if (nextTask->remainingTime == nextTask->period) {
                nextTask->state = TASK_READY;
            }
        } else {
            // 没有任务可执行，可能是系统空闲
        }
    }
}
```

### 4. 任务切换
在任务切换时，可以选择下一个最高优先级的任务，并处理时间片的逻辑。

```c
void Scheduler_Yield(Task_t* currentTask) {
    int nextTaskIndex = FindNextTask(currentTask);
    if (nextTaskIndex != -1) {
        CurrentTaskIndex = nextTaskIndex;
    }
}

int FindNextTask(Task_t* currentTask) {
    int nextTaskIndex = (CurrentTaskIndex + 1) % TaskCount;
    while (TaskList[nextTaskIndex].state != TASK_READY) {
        nextTaskIndex = (nextTaskIndex + 1) % TaskCount;
        if (nextTaskIndex == CurrentTaskIndex) {
            return -1; // 没有可用任务
        }
    }
    return nextTaskIndex;
}
```

### 5. 优先级计算
根据任务周期与截止时间计算优先级，优先级与周期和截止时间成反比。

```c
int CalculatePriority(Task_t* task) {
    // 基于周期的优先级
    int periodPriority = 1000 / task->period; // 假设周期越短，优先级越高

    // 基于截止时间的优先级
    int deadlinePriority = task->deadline - getCurrentTime(); // 假设截止时间越近，优先级越高

    // 综合优先级计算
    int priority = periodPriority * 10 + deadlinePriority; // 权重可以根据需求调整

    return priority;
}
```

### 5. 性能优化
为了进一步提高系统的性能，我们在本周的研究中总结了几个方向，分为以下三点：
• **优先级继承**：在任务调度中引入优先级继承机制，用于避免优先级反转问题。
• **任务合并**：对于具有相同周期和截止时间的任务，可以合并为一个任务以减少调度开销。
• **内存优化**：优化内存分配和释放机制，减少内存管理的开销。
