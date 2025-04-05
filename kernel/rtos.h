#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stack_frame.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
    RTOS_TASKSTATE_WAIT_JOIN,
    RTOS_TASKSTATE_SLEEPING,
    RTOS_TASKSTATE_WAIT_MUTEX,
    RTOS_TASKSTATE_WAIT_COND,
    RTOS_TASKSTATE_WAIT_DEQUEUE,
    RTOS_TASKSTATE_WAIT_ENQUEUE,
} rtos_taskstate_t;

typedef void (*rtos_task_func_t)(void *);

struct rtos_tcb;

typedef struct {
    struct rtos_tcb *head;
    struct rtos_tcb *tail;
} rtos_tlist_t;

typedef struct rtos_tcb {
    switch_frame_t *    switch_frame;
    size_t *            stack_low;
    size_t              priority;
    size_t              def_priority;
    size_t              slice_left;
    size_t              wake_time;
    rtos_taskstate_t    state;
    rtos_tlist_t        waiting_to_join;
    uint8_t *           mqueue_data;
    bool                privileged;
    struct rtos_tcb *   prev;
    struct rtos_tcb *   next;
} rtos_tcb_t;

typedef struct {
    rtos_task_func_t    function;
    void *              task_arg;
    void *              stack_low;
    size_t              stack_size;
    size_t              priority;
    bool                privileged;
} rtos_task_settings_t;

typedef struct {
    rtos_tcb_t *owner;
    rtos_tlist_t blocked;
    size_t priority_ceil;
} rtos_mutex_t;

typedef struct {
    rtos_mutex_t *mutex;
    rtos_tlist_t waiting;
} rtos_cond_t;

void rtos_tick(void);

[[noreturn]] void rtos_start(void);

void rtos_task_create(rtos_tcb_t *task, const rtos_task_settings_t *settings);

rtos_tcb_t *rtos_task_self(void);

[[noreturn]] void rtos_task_exit(void);

void rtos_task_yield(void);

void rtos_task_sleep(size_t ticks);

void rtos_task_suspend(void);

void rtos_task_resume(rtos_tcb_t *task);

void rtos_task_join(rtos_tcb_t *task);

void rtos_mutex_create(rtos_mutex_t *mutex, size_t priority_ceil);
void rtos_mutex_destroy(rtos_mutex_t *mutex);
void rtos_mutex_lock(rtos_mutex_t *mutex);
bool rtos_mutex_trylock(rtos_mutex_t *mutex);
void rtos_mutex_unlock(rtos_mutex_t *mutex);

void rtos_cond_create(rtos_cond_t *cond);
void rtos_cond_destroy(rtos_cond_t *cond);
void rtos_cond_wait(rtos_cond_t *cond, rtos_mutex_t *mutex);
void rtos_cond_signal(rtos_cond_t *cond);
void rtos_cond_broadcast(rtos_cond_t *cond);

typedef struct {
    size_t          slots;
    size_t          slot_size;
    size_t          head;
    size_t          tail;
    bool            is_full;
    rtos_tlist_t    waiting;
    uint8_t *       data;
} rtos_mqueue_t;

void rtos_mqueue_create(rtos_mqueue_t *mqueue, uint8_t *buffer, size_t slots,
                        size_t slot_size);
void rtos_mqueue_destroy(rtos_mqueue_t *mqueue);
void rtos_mqueue_enqueue(rtos_mqueue_t *mqueue, const void *data);
void rtos_mqueue_dequeue(rtos_mqueue_t *mqueue, void *data);
bool rtos_mqueue_try_enqueue_isr(rtos_mqueue_t *mqueue, const void *data);

#ifdef __cplusplus
}
#endif
