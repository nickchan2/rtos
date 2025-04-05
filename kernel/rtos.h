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
    rtos_task_func_t    function;
    void                *task_arg;
    void                *stack_low;
    size_t              stack_size;
    size_t              priority;
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

void rtos_tick(void);

__attribute((noreturn)) void rtos_start(void);

void rtos_task_create(struct rtos_task *task,
                      const struct rtos_task_settings *settings);

struct rtos_task *rtos_task_self(void);

__attribute((noreturn)) void rtos_task_exit(void);

void rtos_task_yield(void);

void rtos_task_sleep(size_t ticks);

void rtos_task_suspend(void);

void rtos_task_resume(struct rtos_task *task);

void rtos_mutex_create(struct rtos_mutex *mutex);
void rtos_mutex_destroy(struct rtos_mutex *mutex);
void rtos_mutex_lock(struct rtos_mutex *mutex);
bool rtos_mutex_trylock(struct rtos_mutex *mutex);
void rtos_mutex_unlock(struct rtos_mutex *mutex);

void rtos_cond_create(struct rtos_cond *cond);
void rtos_cond_destroy(struct rtos_cond *cond);
void rtos_cond_wait(struct rtos_cond *cond, struct rtos_mutex *mutex);
void rtos_cond_signal(struct rtos_cond *cond);
void rtos_cond_broadcast(struct rtos_cond *cond);

void rtos_task_resume_from_isr(struct rtos_task *task);

#ifdef __cplusplus
}
#endif
