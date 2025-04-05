#pragma once

#include "rtos.h"
#include "rtos_assert.h"
#include "stack_frame.h"

// Instead directly entering the task function, a stub function is used which
// calls the task function. This ensures that a task always exits properly.
static void tcb_stub(void *arg, rtos_task_func_t task_func) {
    task_func(arg);
    rtos_task_exit();
    ASSERT(false);
}

static switch_frame_t *tcb_create_switch_frame(
    const rtos_task_settings_t *settings)
{
    const size_t stack_top =
        (size_t)settings->stack_low + settings->stack_size;
    ASSERT(stack_top % 8 == 0);
    switch_frame_nofp_t *const switch_frame =
        (switch_frame_nofp_t *)(stack_top - sizeof(switch_frame_nofp_t));

    *switch_frame = (switch_frame_nofp_t){
        .exc_return = cm4_exc_return_thread_psp_nofp,
        .r0         = (size_t)settings->task_arg,
        .r1         = (size_t)settings->function,
        .pc         = (size_t)tcb_stub,
        .xpsr       = cm4_epsr_thumb_mask,
    };

    return (switch_frame_t *)switch_frame;
}

static void tcb_init(rtos_tcb_t *tcb, const rtos_task_settings_t *settings) {
    *tcb = (rtos_tcb_t){
        .switch_frame       = tcb_create_switch_frame(settings),
        .stack_low          = NULL,
        .priority           = settings->priority,
        .def_priority       = settings->priority,
        .slice_left         = RTOS_TICKS_PER_SLICE,
        .wake_time          = 0,
        .state              = RTOS_TASKSTATE_READY,
        .waiting_to_join    = (rtos_tlist_t){.head = NULL, .tail = NULL},
        .privileged         = settings->privileged,
        .prev               = NULL,
        .next               = NULL,
    };
}

static void tcb_reset_slice(rtos_tcb_t *tcb) {
    tcb->slice_left = RTOS_TICKS_PER_SLICE;
}
