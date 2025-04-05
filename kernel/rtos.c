#include "cortex_m4.h"
#include "rtos.h"
#include "queue.h"
#include "stack_frame.h"
#include "rtos_assert.h"
#include "rtos_state.h"
#include "tcb.h"
#include "tlist.h"
#include "tpq.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static_assert(RTOS_TICKS_PER_SLICE > 0, "Must have at least 1 tick per slice");
static_assert(RTOS_NUM_PRIORITY_LEVELS >= 2, "");

// All of the kernel's state is stored here
static rtos_state_t state = {0};

static inline void pend_context_switch(void) {
    *cm4_icsr |= cm4_icsr_pendsvset_mask;
}

static void idle_task(void *args) {
    while (true) {
        cm4_wait_for_interrupt();
    }
}

static bool preempt_current_task(rtos_tcb_t *task) {
    return task->priority > state.curr_task->priority ||
            state.curr_task == &state.idle_task;
}

static void make_task_ready(rtos_tcb_t *task) {
    task->state = RTOS_TASKSTATE_READY;
    tpq_push_back(&state.ready_tasks, task);

    if (state.curr_task != NULL &&
        preempt_current_task(task) &&
        !state.is_preempting)
    {
        state.is_preempting = true;
        state.curr_task->state = RTOS_TASKSTATE_READY;
        tpq_push_front(&state.ready_tasks, state.curr_task);
        pend_context_switch();
    }
}

/* ----------------------------------------------------------------------------
 * System call implementations
 * ------------------------------------------------------------------------- */

static void prv_start(void) {
    USAGE_ASSERT(state.is_started == false, "RTOS already started");
    state.is_started = true;

    *cm4_fpcsr |= cm4_fpcsr_aspen_mask | cm4_fpcsr_lspen_mask;

    tcb_init(&state.idle_task, &(rtos_task_settings_t){
        .function   = idle_task,
        .task_arg   = NULL,
        .stack_low  = state.idle_task_stack,
        .stack_size = sizeof(state.idle_task_stack),
        .priority   = 0,
    });

    pend_context_switch();
}

static void prv_task_create(rtos_tcb_t *task,
                            const rtos_task_settings_t *settings)
{
    USAGE_ASSERT(settings->function != NULL, "Passed NULL task function");
    USAGE_ASSERT((size_t)settings->stack_low % 8 == 0,
                 "Stack low address must be 8-byte aligned");
    USAGE_ASSERT(settings->stack_size >= 256,
                 "Stack size must be at least 256 bytes");
    USAGE_ASSERT(settings->stack_size % 8 == 0,
                 "Stack size must be multiple of 8");
    USAGE_ASSERT(settings->priority <= RTOS_MAX_TASK_PRIORITY,
                 "Task priority must be at most RTOS_MAX_TASK_PRIORITY");

    tcb_init(task, settings);

    make_task_ready(task);
}

static rtos_tcb_t *prv_task_self(void) {
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    return state.curr_task;
}

static void prv_task_exit(void) {
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");

    while (!tlist_is_empty(&state.curr_task->waiting_to_join)) {
        rtos_tcb_t *const task = tlist_pop_front(&state.curr_task->waiting_to_join);
        ASSERT(task->state == RTOS_TASKSTATE_WAIT_JOIN);
        task->state = RTOS_TASKSTATE_READY;
        tpq_push_back(&state.ready_tasks, task);
    }

    // Setting the current task to NULL indicates that the context switch
    // handler shouldn't save the context of the exited task.
    state.curr_task = NULL;
    pend_context_switch();
}

static void prv_task_yield(void) {
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    tcb_reset_slice(state.curr_task);

    // Only trigger a context switch if there's another ready task.
    if (!tpq_list_is_empty(&state.ready_tasks, state.curr_task)) {
        state.curr_task->state = RTOS_TASKSTATE_READY;
        tpq_push_back(&state.ready_tasks, state.curr_task);
        pend_context_switch();
    }
}

static void prv_task_sleep(size_t ticks) {
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(ticks > 0, "Number of ticks must be greater than zero");

    state.curr_task->wake_time = state.tick_count + ticks;
    state.curr_task->state = RTOS_TASKSTATE_SLEEPING;
    tlist_insert_ascending(&state.sleeping_tasks, state.curr_task);

    pend_context_switch();
}

static void prv_task_suspend(void) {
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    state.curr_task->state = RTOS_TASKSTATE_SUSPENDED;
    pend_context_switch();
}

static void prv_task_resume(rtos_tcb_t *task) {
    USAGE_ASSERT(task != NULL, "Passed NULL task handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    if (task->state == RTOS_TASKSTATE_SUSPENDED) {
        make_task_ready(task);
    }
}

static void prv_task_join(rtos_tcb_t *task) {
    USAGE_ASSERT(task != NULL, "Passed NULL task handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");

    tlist_push_back(&task->waiting_to_join, state.curr_task);
    state.curr_task->state = RTOS_TASKSTATE_WAIT_JOIN;
    pend_context_switch();
}

static void prv_mutex_create(rtos_mutex_t *mutex, size_t priority_ceil) {
    USAGE_ASSERT(priority_ceil <= RTOS_MAX_TASK_PRIORITY, "");
    mutex->owner = NULL;
    mutex->blocked.head = NULL;
    mutex->blocked.tail = NULL;
    mutex->priority_ceil = priority_ceil;
}

static void prv_mutex_destroy(rtos_mutex_t *mutex) {
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex");
    USAGE_ASSERT(mutex->owner == NULL,
                 "Destroying mutex that tasks are still waiting on");
    ASSERT(tlist_is_empty(&mutex->blocked));
}

static bool prv_mutex_trylock(rtos_mutex_t *mutex) {
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(state.curr_task->priority <= mutex->priority_ceil,
                 "Current task's priority is too high to lock mutex");
    USAGE_ASSERT(mutex->owner != state.curr_task,
                 "Attempt to double lock mutex");

    bool got_mutex = false;
    if (mutex->owner == NULL) {
        ASSERT(tlist_is_empty(&mutex->blocked));
        state.curr_task->priority = mutex->priority_ceil;
        mutex->owner = state.curr_task;
        got_mutex = true;
    }
    return got_mutex;
}

static void prv_mutex_lock(rtos_mutex_t *mutex) {
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(state.curr_task->priority <= mutex->priority_ceil, "");
    USAGE_ASSERT(mutex->owner != state.curr_task,
                 "Attempt to double lock mutex");

    if (!prv_mutex_trylock(mutex)) {
        ASSERT(state.curr_task->state == RTOS_TASKSTATE_RUNNING);
        state.curr_task->state = RTOS_TASKSTATE_WAIT_MUTEX;
        state.curr_task->priority = mutex->priority_ceil;
        tlist_push_back(&mutex->blocked, state.curr_task);
        pend_context_switch();
    }
}

static void prv_mutex_unlock(rtos_mutex_t *mutex) {
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(mutex->owner == state.curr_task,
                 "Task other than owner tried to unlock mutex");

    state.curr_task->priority = state.curr_task->def_priority;

    if (tlist_is_empty(&mutex->blocked)) {
        // No tasks were waiting to aquire so the mutex becomes unlocked.
        mutex->owner = NULL;
    } else {
        // Pass the mutex to the unblocked task.
        rtos_tcb_t *const unblocked = tlist_pop_front(&mutex->blocked);
        ASSERT(unblocked->state == RTOS_TASKSTATE_WAIT_MUTEX);
        mutex->owner = unblocked;
        make_task_ready(unblocked);
    }
}

static void prv_cond_create(rtos_cond_t *cond) {
    cond->mutex = NULL;
    cond->waiting.head = NULL;
    cond->waiting.tail = NULL;
}

static void prv_cond_destroy(rtos_cond_t *cond) {
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(tlist_is_empty(&cond->waiting),
                 "Tried to destroy cond while there were tasks waiting on it");
    ASSERT(cond->mutex == NULL);
}

static void prv_cond_wait(rtos_cond_t *cond, rtos_mutex_t *mutex) {
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(cond->mutex == NULL || cond->mutex == mutex, 
                 "The cv is already associated with another mutex");

    prv_mutex_unlock(mutex);

    cond->mutex = mutex;
    state.curr_task->state = RTOS_TASKSTATE_WAIT_COND;
    tlist_push_back(&cond->waiting, state.curr_task);
    pend_context_switch();
}

static void cond_wake_task(rtos_cond_t *cond) {
    rtos_tcb_t *const waken = tlist_pop_front(&cond->waiting);
    ASSERT(waken->state == RTOS_TASKSTATE_WAIT_COND);
    if (cond->mutex->owner == NULL) {
        cond->mutex->owner = waken;
        waken->state = RTOS_TASKSTATE_READY;
        tpq_push_back(&state.ready_tasks, waken);
    } else {
        waken->state = RTOS_TASKSTATE_WAIT_MUTEX;
        tlist_push_back(&cond->mutex->blocked, waken);
    }
}

static void prv_cond_signal(rtos_cond_t *cond) {
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    if (!tlist_is_empty(&cond->waiting)) {
        cond_wake_task(cond);
        if (tlist_is_empty(&cond->waiting)) {
            cond->mutex = NULL;
        }
    }
}

static void prv_cond_broadcast(rtos_cond_t *cond) {
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    while (!tlist_is_empty(&cond->waiting)) {
        cond_wake_task(cond);
    }
    cond->mutex = NULL;
}

static void prv_mqueue_create(rtos_mqueue_t *mqueue, uint8_t *buffer,
                              size_t slots, size_t slot_size)
{
    *mqueue = (rtos_mqueue_t){
        .slots = slots,
        .slot_size = slot_size,
        .head = 0,
        .tail = 0,
        .is_full = false,
        .waiting = {0},
        .data = buffer,
    };
}

static void prv_mqueue_destroy(rtos_mqueue_t *mqueue) {}

static bool mqueue_try_enqueue(rtos_mqueue_t *mqueue, const void *data) {
    bool success = false;
    if (!queue_is_full(mqueue)) {
        if (tlist_is_empty(&mqueue->waiting)) {
            queue_enqueue(mqueue, data);
        } else {
            rtos_tcb_t *const waken = tlist_pop_front(&mqueue->waiting);
            ASSERT(waken->state == RTOS_TASKSTATE_WAIT_DEQUEUE);
            for (size_t i = 0; i < mqueue->slot_size; ++i) {
                waken->mqueue_data[i] = ((uint8_t *)data)[i];
            }
            make_task_ready(waken);
        }
        success = true;
    }
    return success;
}

static void prv_mqueue_enqueue(rtos_mqueue_t *mqueue, const void *data) {
    if (!mqueue_try_enqueue(mqueue, data)) {
        ASSERT(state.curr_task->state == RTOS_TASKSTATE_RUNNING);
        state.curr_task->state = RTOS_TASKSTATE_WAIT_ENQUEUE;
        tlist_push_back(&mqueue->waiting, state.curr_task);
        state.curr_task->mqueue_data = (void *)data;
        pend_context_switch();
    }
}

static void prv_mqueue_dequeue(rtos_mqueue_t *mqueue, void *data) {
    if (!queue_is_empty(mqueue)) {
        queue_dequeue(mqueue, data);
        if (!tlist_is_empty(&mqueue->waiting)) {
            rtos_tcb_t *const waken = tlist_pop_front(&mqueue->waiting);
            ASSERT(waken->state == RTOS_TASKSTATE_WAIT_ENQUEUE);
            queue_enqueue(mqueue, waken->mqueue_data);
            make_task_ready(waken);
        }
    } else {
        ASSERT(state.curr_task->state == RTOS_TASKSTATE_RUNNING);
        state.curr_task->state = RTOS_TASKSTATE_WAIT_DEQUEUE;
        tlist_push_back(&mqueue->waiting, state.curr_task);
        state.curr_task->mqueue_data = data;
        pend_context_switch();
    }
}

/* ----------------------------------------------------------------------------
 * Interrupt handlers
 * ------------------------------------------------------------------------- */

[[gnu::used]] static void svc_handler_main(exception_entry_stack_t *stack) {
    // The SVC number is encoded in the low byte of the SVC instruction. To
    // access it, the PC saved on the stack during exception entry is used.
    const int svc_num = ((uint8_t *)stack->pc)[-2];
    size_t rv = 0;
    switch (svc_num) {
        case 0:
            prv_start();
            break;
        case 1:
            prv_task_create((rtos_tcb_t *)stack->r0,
                            (const rtos_task_settings_t *)stack->r1);
            break;
        case 2:
            rv = (size_t)prv_task_self();
            break;
        case 3:
            prv_task_exit();
            break;
        case 4:
            prv_task_yield();
            break;
        case 5:
            prv_task_sleep(stack->r0);
            break;
        case 6:
            prv_task_suspend();
            break;
        case 7:
            prv_task_resume((void *)stack->r0);
            break;
        case 8:
            prv_task_join((void *)stack->r0);
            break;
        case 9:
            prv_mutex_create((void *)stack->r0, stack->r1);
            break;
        case 10:
            prv_mutex_destroy((void *)stack->r0);
            break;
        case 11:
            prv_mutex_lock((void *)stack->r0);
            break;
        case 12:
            rv = prv_mutex_trylock((void *)stack->r0);
            break;
        case 13:
            prv_mutex_unlock((void *)stack->r0);
            break;
        case 14:
            prv_cond_create((void *)stack->r0);
            break;
        case 15:
            prv_cond_destroy((void *)stack->r0);
            break;
        case 16:
            prv_cond_wait((void *)stack->r0, (void *)stack->r1);
            break;
        case 17:
            prv_cond_signal((void *)stack->r0);
            break;
        case 18:
            prv_cond_broadcast((void *)stack->r0);
            break;
        case 19:
            prv_mqueue_create((void *)stack->r0, (void *)stack->r1, stack->r2,
                              stack->r3);
            break;
        case 20:
            prv_mqueue_destroy((void *)stack->r0);
            break;
        case 21:
            prv_mqueue_enqueue((void *)stack->r0, (void *)stack->r1);
            break;
        case 22:
            prv_mqueue_dequeue((void *)stack->r0, (void *)stack->r1);
            break;
        default:
#ifdef RTOS_DEBUG
            size_t debug_syscall(void *, int);
            rv = debug_syscall((void *)stack->r0, svc_num);
#else // ifdef RTOS_DEBUG
            USAGE_ASSERT(false, "Invalid SVC number");
#endif // ifdef RTOS_DEBUG
            break;
    }

    // Store the return value in the part of the stack that gets popped to R0
    // when the handler returns.
    stack->r0 = rv;
}

[[gnu::naked]] void SVC_Handler(void) {
    __asm volatile(
    "   tst     lr, #4              \n"
    "   ite     eq                  \n"
    "   mrseq   r0, msp             \n"
    "   mrsne   r0, psp             \n"
    "   b       svc_handler_main    \n"
    );
}

// Returns a pointer to the next task's switch frame.
[[gnu::used]] static switch_frame_t *choose_next_task(
    switch_frame_t *old_switch_frame)
{
    if (state.curr_task != NULL) {
        USAGE_ASSERT((size_t)old_switch_frame >=
                         (size_t)state.curr_task->stack_low,
                     "Task stack overflow");
        ASSERT(state.curr_task->state != RTOS_TASKSTATE_RUNNING);
        state.curr_task->switch_frame = old_switch_frame;
    }

    // Choose the highest priority task that's ready to run next.
    rtos_tcb_t *const next_task = tpq_pop_front(&state.ready_tasks) 
                                    ?: &state.idle_task;

    size_t control = cm4_get_control();
    if (next_task->privileged) {
        control |= cm4_control_npriv_mask;
    } else {
        control &= ~cm4_control_npriv_mask;
    }
    cm4_set_control(control);

    ASSERT(next_task->state == RTOS_TASKSTATE_READY);
    next_task->state = RTOS_TASKSTATE_RUNNING;
    state.is_preempting = false;
    state.curr_task = next_task;
    return next_task->switch_frame;
}

// Interrupts are disabled during this to ensure an atomic context switch. This
// is needed because PendSV has the lowest priority and otherwise, another
// interrupt could pre-empt this handler and call a kernel function while the
// kernel state is invalid.
static_assert(NULL == 0, "Assembly assumes NULL == 0");
[[gnu::naked]] void PendSV_Handler(void) {
    __asm volatile(
    "   cpsid       i               \n" // Disable interrupts to ensure atomic context switch.
    "                               \n"
    "   ldr         r1, =state      \n" // r1 = &state
    "                               \n"
    "   ldr         r2, [r1, #0]    \n" // r2 = state.curr_task
    "   cmp         r2, #0          \n" // Check if regs should be saved.
    "   it          eq              \n"
    "   beq         no_save         \n"
    "                               \n"
    "   mrs         r0, psp         \n" // r0 = PSP (current task's stack)
    "   stmdb       r0!, {r4-r11}   \n" // Save remaining general-purpose regs
    "   tst         lr, #0x10       \n"
    "   it          eq              \n"
    "   vstmdbeq    r0!, {s16-s31}  \n" // Save remaining FP regs if used
    "   sub         r0, #4          \n"
    "   str         lr, [r0]        \n" // Save EXC_RETURN
    "                               \n"
    "no_save:                       \n"
    "   bl          choose_next_task\n" // r0 = choose_next_task(r0)
    "   ldr         lr, [r0]        \n" // Restore EXC_RETURN
    "   add         r0, #4          \n"
    "   mrs         r1, control     \n" // r1 = control
    "   tst         lr, #0x10       \n" // Check if FP was used
    "   it          eq              \n"
    "   beq         restore_fp      \n"
    "   and         r1, #0xFFFFFFFB \n" // Clear FP bit
    "   b           no_fp           \n"
    "restore_fp:                    \n"
    "   vldmia      r0!, {s16-s31}  \n" // Restore FP regs
    "   orr         r1, #0x4        \n" // Set FP bit
    "no_fp:                         \n"
    "   ldmia       r0!, {r4-r11}   \n" // Restore general-purpose regs
    "   msr         psp, r0         \n" // Update the PSP
    "   msr         control, r1     \n" // Update CONTROL
    "                               \n"
    "   cpsie       i               \n" // Re-enable interrupts
    "   bx          lr              \n"
    );
}

void rtos_tick(void) {
    if (!state.is_started) {
        return;
    }

    ++state.tick_count;
    --state.curr_task->slice_left;

    bool context_switch_required = false;
    bool push_to_back = false;

    // Check if the current task's time slice expired.
    if (state.curr_task->slice_left == 0) {
        state.curr_task->slice_left = RTOS_TICKS_PER_SLICE;
        if (!tpq_list_is_empty(&state.ready_tasks, state.curr_task)) {
            context_switch_required = true;
            push_to_back = true;
        }
    }

    // Check if there are any sleeping tasks to wake.
    while (!tlist_is_empty(&state.sleeping_tasks) &&
           state.sleeping_tasks.head->wake_time <= state.tick_count)
    {
        rtos_tcb_t *const waken = tlist_pop_front(&state.sleeping_tasks);
        ASSERT(waken->state == RTOS_TASKSTATE_SLEEPING);
        waken->state = RTOS_TASKSTATE_READY;
        tpq_push_back(&state.ready_tasks, waken);

        // Check if the waken task preempts the current task.
        if (waken->priority > state.curr_task->priority) {
            context_switch_required = true;
        }
    }

    if (context_switch_required) {
        state.curr_task->state = RTOS_TASKSTATE_READY;
        if (push_to_back) {
            tpq_push_back(&state.ready_tasks, state.curr_task);
        } else {
            tpq_push_front(&state.ready_tasks, state.curr_task);
        }
        pend_context_switch();
    }
}

/* ----------------------------------------------------------------------------
 * Public API implementations
 * ------------------------------------------------------------------------- */

// Macro for SVC wrapper function implementations.
#define svccall(num, name, ret, ...) \
    [[gnu::naked]] ret name(__VA_ARGS__) {          \
        __asm volatile(                             \
        "   svc    "#num"   \n"                     \
        "   bx      lr      \n"                     \
        );                                          \
    }

svccall(0,  rtos_start,         void,   void)
svccall(1,  rtos_task_create,   void,   rtos_tcb_t *task,
                                        const rtos_task_settings_t *settings)
svccall(2,  rtos_task_self,     rtos_tcb_t *, void)
svccall(3,  rtos_task_exit,     void,   void)
svccall(4,  rtos_task_yield,    void,   void)
svccall(5,  rtos_task_sleep,    void,   size_t ticks)
svccall(6,  rtos_task_suspend,  void,   void)
svccall(7,  rtos_task_resume,   void,   rtos_tcb_t *task)
svccall(8,  rtos_task_join,     void,   rtos_tcb_t *task)
svccall(9,  rtos_mutex_create,  void,   rtos_mutex_t *mutex,
                                        size_t priority_ceil)
svccall(10, rtos_mutex_destroy, void,   rtos_mutex_t *task)
svccall(11, rtos_mutex_lock,    void,   rtos_mutex_t *task)
svccall(12, rtos_mutex_trylock, bool,   rtos_mutex_t *task)
svccall(13, rtos_mutex_unlock,  void,   rtos_mutex_t *task)
svccall(14, rtos_cond_create,   void,   rtos_cond_t *cond)
svccall(15, rtos_cond_destroy,  void,   rtos_cond_t *cond)
svccall(16, rtos_cond_wait,     void,   rtos_cond_t *cond,
                                        rtos_mutex_t *mutex)
svccall(17, rtos_cond_signal,   void,   rtos_cond_t *cond)
svccall(18, rtos_cond_broadcast,void,   rtos_cond_t *cond)
svccall(19, rtos_mqueue_create, void,   rtos_mqueue_t *mqueue, uint8_t *buffer,
                                        size_t slots, size_t slot_size)
svccall(20, rtos_mqueue_destroy,void,   rtos_mqueue_t *mqueue)
svccall(21, rtos_mqueue_enqueue,void,   rtos_mqueue_t *mqueue,
                                        const void *data)
svccall(22, rtos_mqueue_dequeue,void,   rtos_mqueue_t *mqueue,
                                        void *data)

bool rtos_mqueue_try_enqueue_isr(rtos_mqueue_t *mqueue, const void *data) {
    cm4_disable_irq();
    bool success = mqueue_try_enqueue(mqueue, data);
    cm4_enable_irq();
    return success;
}
