## `rtos_tick`

Call this function from SysTick_Handler. Increments the kernel's internal tick
count and may cause a context switch.

## `rtos_start`

Start the RTOS and enter the first task.

## `rtos_task_create`

Create a new task. Can be called before RTOS is started.

Parameters:
- `task: struct rtos_task *`
    - Handle to the task to create.
- `settings: struct rtos_task_settings *`
    - Settings for the task.
    - `function`: Task function pointer
    - `task_arg`: Pointer to pass to the task function
    - `stack_low`: Low address of the task's stack. Must be aligned to 8 bytes.
    - `stack_size`: Size of the stack in bytes. Must be at least 256 and a
                    multiple of 8.
    - `priority`: The priority of the task. Higher number means higher
                  priority. Must be in the range [1, RTOS_MAX_TASK_PRIORITY].
                  Note that 0 is reserved for the idle task.

## `rtos_task_self`

Get the handle of the currently running task. Do not call before RTOS is
started.

Returns: `struct rtos_task *`
- Handle of the currently running task.

## `rtos_task_exit`

Exit the currently running task. Do not call before RTOS is started.

## `rtos_task_yield`

Yield the CPU to the next task (at the same priority level as the current
task). The yielding task's time slice is reset. Do not call before RTOS is
started.

## `rtos_task_sleep`

The currently running task sleeps for a given number of ticks. The task's time
slice is reset. Do not call before RTOS is started.

Parameters:
- `ticks: size_t`
    - Number of ticks to sleep.

## `rtos_task_suspend`

Suspend the currently running task. Do not call before RTOS is started.

## `rtos_task_resume`

Resume a suspended task. Do not call before RTOS is started.

Parameters:
- `task: struct rtos_task *`
    - Handle of the task to resume.
