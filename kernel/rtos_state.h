#pragma once

#include "rtos.h"
#include "tpq.h"

#include <stdint.h>

typedef struct {
    // Accessed from assembly begin
    rtos_tcb_t *    curr_task;
    // Accessed from assembly end
    bool            is_started;
    bool            is_preempting; // TODO: Is there a better alternative?
    size_t          tick_count;
    rtos_tpq_t      ready_tasks;
    rtos_tlist_t    sleeping_tasks;
    rtos_tcb_t      idle_task;
    uint8_t         idle_task_stack[256] __attribute__((aligned(8)));
} rtos_state_t;
