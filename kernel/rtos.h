#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>

/* The RTOS includes optional assertions to ensure that it is being used
 * correctly. If enabled, if an assertion fails, an infinite spin is entered.
 * The cause can be determined using a debugger and examining arguments the
 * infinite spin function */
#ifndef RTOS_ENABLE_USAGE_ASSERT
#define RTOS_ENABLE_USAGE_ASSERT 1
#endif

enum {
    RTOS_TICKS_PER_SLICE        = 10,
    RTOS_NUM_PRIORITY_LEVELS    = 4,
    RTOS_MAX_TASK_PRIORITY      = RTOS_NUM_PRIORITY_LEVELS - 1,
};
static_assert(RTOS_TICKS_PER_SLICE > 0, "Must have at least 1 tick per slice");
static_assert(RTOS_NUM_PRIORITY_LEVELS >= 2, "");

enum rtos_taskstate {
    RTOS_TASKSTATE_RUNNING,
    RTOS_TASKSTATE_READY,
    RTOS_TASKSTATE_SUSPENDED,
    RTOS_TASKSTATE_SLEEPING,
    RTOS_TASKSTATE_WAIT_MUTEX,
    RTOS_TASKSTATE_WAIT_COND,
};

typedef void (*rtos_task_func_t)(void *);

struct rtos_task {
    size_t              *switch_frame;
    size_t              *stack_low;
    size_t              priority;
    size_t              slice_left;
    size_t              wake_time;
    enum rtos_taskstate state;
    struct rtos_task    *prev;
    struct rtos_task    *next;
};

struct rtos_task_settings {
    rtos_task_func_t function; // Pointer to the task function
    void *task_arg; // Pointer to pass to the task function
    void *stack_low; // Low address of the stack. Must be aligned to 8 bytes.
    size_t stack_size;  // Size of the stack in bytes. Must be at least 256 and a multiple of 8.
    size_t priority; // The priority of the task. Higher number means higher priority. Must be in the range [1, RTOS_MAX_TASK_PRIORITY]. Note that 0 is reserved for the idle task.
};

struct rtos_dll {
    struct rtos_task *head;
    struct rtos_task *tail;
};

struct rtos_mutex {
    struct rtos_task *owner;
    struct rtos_dll blocked;
};

struct rtos_cond {
    struct rtos_mutex *mutex;
    struct rtos_dll waiting;
};

/*
 * Call this function from SysTick_Handler. Increments the kernel's internal
 * tick count and may cause a context switch.
 */
void rtos_tick(void);

/*
 * Start the RTOS and enter the first task. This function never returns.
 */
__attribute((noreturn)) void rtos_start(void);

/*
 * Create a new task. Can be called before RTOS is started.
 * Parameters:
 *     - task: Handle to the task to create.
 *     - settings: Settings for the task.
 */
void rtos_task_create(struct rtos_task *task,
                      const struct rtos_task_settings *settings);

/*
 * Get the handle of the currently running task. Do not call before RTOS is
 * started.
 * Return:
 *     - Handle of the currently running task.
 */
struct rtos_task *rtos_task_self(void);

/*
 * Exit the currently running task. Do not call before RTOS is started.
 */
 __attribute((noreturn)) void rtos_task_exit(void);

/*
 * Yield the CPU to the next task (at the same priority level as the current
 * task). The yielding task's time slice is reset. Do not call before RTOS is
 * started.
 */
void rtos_task_yield(void);

/*
 * The currently running task sleeps for a given number of ticks. The task's
 * time slice is reset. Do not call before RTOS is started.
 * Parameters:
 *     - ticks: Number of ticks to sleep.
 */
void rtos_task_sleep(size_t ticks);

/*
 * Suspend the currently running task. Do not call before RTOS is started.
 */
void rtos_task_suspend(void);

/*
 * Resume a suspended task. Do not call before RTOS is started.
 * Parameters:
 *     - task: Handle of the task to resume.
 */
void rtos_task_resume(struct rtos_task *task);

/*
 * Create a mutex. Can be called before RTOS is started.
 */
void rtos_mutex_create(struct rtos_mutex *);
void rtos_mutex_destroy(struct rtos_mutex *mutex);
void rtos_mutex_lock(struct rtos_mutex *mutex);
bool rtos_mutex_trylock(struct rtos_mutex *mutex);
void rtos_mutex_unlock(struct rtos_mutex *mutex);

void rtos_cond_create(struct rtos_cond *cond);
void rtos_cond_destroy(struct rtos_cond *cond);
void rtos_cond_wait(struct rtos_cond *cond, struct rtos_mutex *mutex);
void rtos_cond_signal(struct rtos_cond *cond);
void rtos_cond_broadcast(struct rtos_cond *cond);

/*
 * Resume a suspended task from an ISR.
 */
void rtos_task_resume_from_isr(struct rtos_task *task);

#ifdef __cplusplus
}
#endif
