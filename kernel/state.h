#pragma once

#include "rtos.h"

#include <stdint.h>

typedef struct {
    // Accessed from assembly begin
    rtos_task_t *curr_task;
    // Accessed from assembly end
    bool        is_started;
    bool        is_preempting; // TODO: Is there a better alternative?
    size_t      tick_count;
    rtos_dll_t  ready_task_q[RTOS_NUM_PRIORITY_LEVELS];
    rtos_dll_t  sleeping_tasks;
    rtos_task_t idle_task;
    uint8_t     idle_task_stack[256] __attribute__((aligned(8)));
} rtos_state_t;
