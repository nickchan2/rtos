#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#ifndef RTOS_ENABLE_USAGE_ASSERT
#define RTOS_ENABLE_USAGE_ASSERT 1
#endif

#ifndef RTOS_TICKS_PER_SLICE
#define RTOS_TICKS_PER_SLICE 10
#endif

#ifndef RTOS_NUM_PRIORITY_LEVELS
#define RTOS_NUM_PRIORITY_LEVELS 3
#endif

enum {
    RTOS_MAX_TASK_PRIORITY = RTOS_NUM_PRIORITY_LEVELS - 1,
};

typedef enum {
    RTOS_TASKSTATE_RUNNING,
    RTOS_TASKSTATE_READY,
    RTOS_TASKSTATE_SUSPENDED,
    RTOS_TASKSTATE_SLEEPING,
    RTOS_TASKSTATE_WAIT_MUTEX,
    RTOS_TASKSTATE_WAIT_COND,
} rtos_taskstate_t;

typedef void (*rtos_task_func_t)(void *);

typedef struct rtos_task {
    size_t *            switch_frame;
    size_t *            stack_low;
    size_t              priority;
    size_t              def_priority;
    size_t              slice_left;
    size_t              wake_time;
    rtos_taskstate_t    state;
    struct rtos_task *  prev;
    struct rtos_task *  next;
} rtos_task_t;

typedef struct {
    rtos_task_func_t    function;
    void *              task_arg;
    void *              stack_low;
    size_t              stack_size;
    size_t              priority;
} rtos_task_settings_t;

typedef struct {
    rtos_task_t *head;
    rtos_task_t *tail;
} rtos_dll_t;

struct rtos_mutex {
    struct rtos_task *owner;
    rtos_dll_t blocked;
    size_t priority_ceil;
};

struct rtos_cond {
    struct rtos_mutex *mutex;
    rtos_dll_t waiting;
};

void rtos_tick(void);

[[noreturn]] void rtos_start(void);

void rtos_task_create(rtos_task_t *task, const rtos_task_settings_t *settings);

struct rtos_task *rtos_task_self(void);

[[noreturn]] void rtos_task_exit(void);

void rtos_task_yield(void);

void rtos_task_sleep(size_t ticks);

void rtos_task_suspend(void);

void rtos_task_resume(struct rtos_task *task);

void rtos_mutex_create(struct rtos_mutex *mutex, size_t priority_ceil);
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
