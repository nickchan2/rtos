#include "cortex_m4.h"
#include "dll.h"
#include "rtos.h"
#include "rtos_assert.h"
#include "stack_frame.h"
#include "state.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static_assert(RTOS_TICKS_PER_SLICE > 0, "Must have at least 1 tick per slice");
static_assert(RTOS_NUM_PRIORITY_LEVELS >= 2, "");

// All of the kernel's state is stored here
static rtos_state_t state = {0};

static void prv_task_create(rtos_task_t *, const rtos_task_settings_t *);

static inline void pend_context_switch(void)
{
    *cm4_icsr |= cm4_icsr_pendsvset_mask;
}

// Instead directly entering the task function, a stub function is used which
// calls the task function. This ensures that a task always exits properly.
static void stub(void *arg, rtos_task_func_t task_func)
{
    task_func(arg);
    rtos_task_exit();
    ASSERT(false);
}

static void idle_task(void *args)
{
    while (true) {
        cm4_wait_for_interrupt();
    }
}

static size_t *create_switch_frame(const rtos_task_settings_t *settings)
{
    void *const stack_top = settings->stack_low + settings->stack_size;
    USAGE_ASSERT(((size_t)stack_top & 0b111) == 0,
                  "Stack must be 8-byte aligned");
    switch_frame_nofp_t *const switch_frame = stack_top - sizeof(switch_frame_nofp_t);

    *switch_frame = (switch_frame_nofp_t){
        .exc_return = cm4_exc_return_thread_psp_nofp,
        .r0         = (size_t)settings->task_arg,
        .r1         = (size_t)settings->function,
        .pc         = (size_t)stub,
        .xpsr       = cm4_epsr_thumb_mask,
    };

    return (void *)switch_frame;
}

static void task_init(rtos_task_t *task, const rtos_task_settings_t *settings)
{
    *task = (rtos_task_t){
        .switch_frame   = create_switch_frame(settings),
        .stack_low      = NULL,
        .priority       = settings->priority,
        .def_priority   = settings->priority,
        .slice_left     = RTOS_TICKS_PER_SLICE,
        .wake_time      = 0,
        .state          = RTOS_TASKSTATE_READY,
        .prev           = NULL,
        .next           = NULL,
    };
}

static void make_task_ready(rtos_task_t *task)
{
    task->state = RTOS_TASKSTATE_READY;
    dll_push_back(&state.ready_task_q[task->priority], task);

    if (state.curr_task != NULL &&
        task->priority > state.curr_task->priority &&
        !state.is_preempting)
    {
        state.is_preempting = true;
        state.curr_task->state = RTOS_TASKSTATE_READY;
        dll_push_front(&state.ready_task_q[state.curr_task->priority],
                        state.curr_task);
        pend_context_switch();
    }
}

/* ----------------------------------------------------------------------------
 * System call implementations
 * ------------------------------------------------------------------------- */

static void prv_start(void)
{
    USAGE_ASSERT(state.is_started == false, "RTOS already started");
    state.is_started = true;

    // FIXME
    static volatile size_t *const FPCSR = (volatile size_t *)0xE000EF34U;
    *FPCSR |= (1 << 31) | (1 << 30); // ASPEN | LSPEN

    task_init(&state.idle_task, &(rtos_task_settings_t){
        .function = idle_task,
        .task_arg = NULL,
        .stack_low = state.idle_task_stack,
        .stack_size = sizeof(state.idle_task_stack),
        .priority = 0,
    });

    pend_context_switch();
}

static void prv_task_create(rtos_task_t *task,
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

    task_init(task, settings);

    make_task_ready(task);
}

static rtos_task_t *prv_task_self(void)
{
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    return state.curr_task;
}

static void prv_task_exit(void)
{
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");

    // Setting the current task to NULL indicates that the context switch
    // handler shouldn't save the context of the exited task.
    state.curr_task = NULL;
    pend_context_switch();
}

static void prv_task_yield(void)
{
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    state.curr_task->slice_left = RTOS_TICKS_PER_SLICE;

    // Only trigger a context switch if there's another ready task.
    if (!dll_is_empty(&state.ready_task_q[state.curr_task->priority])) {
        state.curr_task->state = RTOS_TASKSTATE_READY;
        dll_push_back(&state.ready_task_q[state.curr_task->priority],
                      state.curr_task);
        pend_context_switch();
    }
}

static void prv_task_sleep(size_t ticks)
{
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(ticks > 0, "Number of ticks must be greater than zero");

    state.curr_task->wake_time = state.tick_count + ticks;
    state.curr_task->state = RTOS_TASKSTATE_SLEEPING;
    dll_insert_ascending(&state.sleeping_tasks, state.curr_task);

    pend_context_switch();
}

static void prv_task_suspend(void)
{
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    state.curr_task->state = RTOS_TASKSTATE_SUSPENDED;
    pend_context_switch();
}

static void prv_task_resume(rtos_task_t *task)
{
    USAGE_ASSERT(task != NULL, "Passed NULL task handle");
    if (!state.is_started || task->state != RTOS_TASKSTATE_SUSPENDED) {
        return;
    }
    make_task_ready(task);
}

static void prv_mutex_create(struct rtos_mutex *mutex, size_t priority_ceil)
{
    USAGE_ASSERT(priority_ceil <= RTOS_MAX_TASK_PRIORITY, "");
    mutex->owner = NULL;
    mutex->blocked.head = NULL;
    mutex->blocked.tail = NULL;
    mutex->priority_ceil = priority_ceil;
}

static void prv_mutex_destroy(struct rtos_mutex *mutex)
{
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex");
    USAGE_ASSERT(mutex->owner == NULL,
                 "Destroying mutex that tasks are still waiting on");
    ASSERT(dll_is_empty(&mutex->blocked));
}

static bool prv_mutex_trylock(struct rtos_mutex *mutex)
{
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(state.curr_task->priority <= mutex->priority_ceil,
                 "Current task's priority is too high to lock mutex");
    USAGE_ASSERT(mutex->owner != state.curr_task,
                 "Attempt to double lock mutex");

    bool got_mutex = false;
    if (mutex->owner == NULL) {
        ASSERT(dll_is_empty(&mutex->blocked));
        state.curr_task->priority = mutex->priority_ceil;
        mutex->owner = state.curr_task;
        got_mutex = true;
    }
    return got_mutex;
}

static void prv_mutex_lock(struct rtos_mutex *mutex)
{
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(state.curr_task->priority <= mutex->priority_ceil, "");
    USAGE_ASSERT(mutex->owner != state.curr_task,
                 "Attempt to double lock mutex");

    if (!prv_mutex_trylock(mutex)) {
        ASSERT(state.curr_task->state == RTOS_TASKSTATE_RUNNING);
        state.curr_task->state = RTOS_TASKSTATE_WAIT_MUTEX;
        state.curr_task->priority = mutex->priority_ceil;
        dll_push_back(&mutex->blocked, state.curr_task);
        pend_context_switch();
    }
}

static void prv_mutex_unlock(struct rtos_mutex *mutex)
{
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(mutex->owner == state.curr_task,
                 "Task other than owner tried to unlock mutex");

    state.curr_task->priority = state.curr_task->def_priority;

    if (dll_is_empty(&mutex->blocked)) {
        // No tasks were waiting to aquire so the mutex becomes unlocked.
        mutex->owner = NULL;
    } else {
        // Pass the mutex to the unblocked task.
        rtos_task_t *const unblocked = dll_pop_front(&mutex->blocked);
        ASSERT(unblocked->state == RTOS_TASKSTATE_WAIT_MUTEX);
        mutex->owner = unblocked;
        make_task_ready(unblocked);
    }
}

static void prv_cond_create(struct rtos_cond *cond)
{
    cond->mutex = NULL;
    cond->waiting.head = NULL;
    cond->waiting.tail = NULL;
}

static void prv_cond_destroy(struct rtos_cond *cond)
{
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(dll_is_empty(&cond->waiting),
                 "Tried to destroy cond while there were tasks waiting on it");
    ASSERT(cond->mutex == NULL);
}

static void prv_cond_wait(struct rtos_cond *cond, struct rtos_mutex *mutex)
{
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(mutex != NULL, "Passed NULL mutex handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    USAGE_ASSERT(cond->mutex == NULL || cond->mutex == mutex, 
                 "The cv is already associated with another mutex");

    prv_mutex_unlock(mutex);

    cond->mutex = mutex;
    state.curr_task->state = RTOS_TASKSTATE_WAIT_COND;
    dll_push_back(&cond->waiting, state.curr_task);
    pend_context_switch();
}

static void cond_wake_task(struct rtos_cond *cond)
{
    rtos_task_t *const waken = dll_pop_front(&cond->waiting);
    ASSERT(waken->state == RTOS_TASKSTATE_WAIT_COND);
    if (cond->mutex->owner == NULL) {
        cond->mutex->owner = waken;
        waken->state = RTOS_TASKSTATE_READY;
        dll_push_back(&state.ready_task_q[waken->priority], waken);
    } else {
        waken->state = RTOS_TASKSTATE_WAIT_MUTEX;
        dll_push_back(&cond->mutex->blocked, waken);
    }
}

static void prv_cond_signal(struct rtos_cond *cond)
{
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    if (!dll_is_empty(&cond->waiting)) {
        cond_wake_task(cond);
    }
    if (dll_is_empty(&cond->waiting)) {
        cond->mutex = NULL;
    }
}

static void prv_cond_broadcast(struct rtos_cond *cond)
{
    USAGE_ASSERT(cond != NULL, "Passed NULL cond handle");
    USAGE_ASSERT(state.is_started, "RTOS must be started before calling");
    while (!dll_is_empty(&cond->waiting)) {
        cond_wake_task(cond);
    }
    cond->mutex = NULL;
}

/* ----------------------------------------------------------------------------
 * Interrupt handlers
 * ------------------------------------------------------------------------- */

[[gnu::used]] static void svc_handler_main(exception_entry_stack_t *stack)
{
    // The SVC number is encoded in the low byte of the SVC instruction. To
    // access it, the PC saved on the stack during exception entry is used.
    const int svc_num = ((uint8_t *)stack->pc)[-2];
    size_t rv = 0;
    switch (svc_num) {
        case 0:
            prv_start();
            break;
        case 1:
            prv_task_create((rtos_task_t *)stack->r0,
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
            prv_mutex_create((void *)stack->r0, stack->r1);
            break;
        case 9:
            prv_mutex_destroy((void *)stack->r0);
            break;
        case 10:
            prv_mutex_lock((void *)stack->r0);
            break;
        case 11:
            rv = prv_mutex_trylock((void *)stack->r0);
            break;
        case 12:
            prv_mutex_unlock((void *)stack->r0);
            break;
        case 13:
            prv_cond_create((void *)stack->r0);
            break;
        case 14:
            prv_cond_destroy((void *)stack->r0);
            break;
        case 15:
            prv_cond_wait((void *)stack->r0, (void *)stack->r1);
            break;
        case 16:
            prv_cond_signal((void *)stack->r0);
            break;
        case 17:
            prv_cond_broadcast((void *)stack->r0);
            break;
        default:
            ASSERT(false);
            break;
    }

    // Store the return value in the part of the stack that gets popped to R0
    // when the handler returns.
    stack->r0 = rv;
}

[[gnu::naked]] void SVC_Handler(void)
{
    __asm volatile(
    "   tst     lr, #4              \n"
    "   ite     eq                  \n"
    "   mrseq   r0, msp             \n"
    "   mrsne   r0, psp             \n"
    "   b       svc_handler_main    \n"
    );
}

// Returns a pointer to the next task's switch frame.
[[gnu::used]] static void *choose_next_task(void *old_switch_frame)
{
    if (state.curr_task != NULL) {
        USAGE_ASSERT((size_t)old_switch_frame >=
                         (size_t)state.curr_task->stack_low,
                     "Task stack overflow");
        ASSERT(state.curr_task->state != RTOS_TASKSTATE_RUNNING);
        state.curr_task->switch_frame = old_switch_frame;
    }

    // Choose the highest priority task that's ready to run next.
    rtos_task_t *next_task = NULL;
    for (int i = RTOS_MAX_TASK_PRIORITY; i > -1; --i) {
        if (!dll_is_empty(&state.ready_task_q[i])) {
            next_task = dll_pop_front(&state.ready_task_q[i]);
            break;
        }
    }
    if (next_task == NULL) {
        // No tasks are ready to run. The idle task will be run instead.
        next_task = &state.idle_task;
    }

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
[[gnu::naked]] void PendSV_Handler(void)
{
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

void rtos_tick(void)
{
    if (!state.is_started) {
        return;
    }

    ++state.tick_count;
    --state.curr_task->slice_left;

    bool context_switch_required = false;

    // Check if the current task's time slice expired.
    if (state.curr_task->slice_left == 0) {
        state.curr_task->slice_left = RTOS_TICKS_PER_SLICE;
        if (!dll_is_empty(&state.ready_task_q[state.curr_task->priority])) {
            context_switch_required = true;
        }
    }

    // Check if there are any sleeping tasks to wake.
    while (!dll_is_empty(&state.sleeping_tasks) &&
           state.sleeping_tasks.head->wake_time <= state.tick_count)
    {
        rtos_task_t *const waken = dll_pop_front(&state.sleeping_tasks);
        ASSERT(waken->state == RTOS_TASKSTATE_SLEEPING);
        waken->state = RTOS_TASKSTATE_READY;
        dll_push_back(&state.ready_task_q[waken->priority], waken);

        // Check if the waken task preempts the current task.
        if (waken->priority > state.curr_task->priority) {
            context_switch_required = true;
        }
    }

    if (context_switch_required) {
        state.curr_task->state = RTOS_TASKSTATE_READY;
        dll_push_back(&state.ready_task_q[state.curr_task->priority],
                      state.curr_task);
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
svccall(1,  rtos_task_create,   void,   rtos_task_t *task,
                                        const rtos_task_settings_t *settings)
svccall(2,  rtos_task_self,     rtos_task_t *, void)
svccall(3,  rtos_task_exit,     void,   void)
svccall(4,  rtos_task_yield,    void,   void)
svccall(5,  rtos_task_sleep,    void,   size_t ticks)
svccall(6,  rtos_task_suspend,  void,   void)
svccall(7,  rtos_task_resume,   void,   rtos_task_t *task)
svccall(8,  rtos_mutex_create,  void,   struct rtos_mutex *mutex,
                                        size_t priority_ceil)
svccall(9,  rtos_mutex_destroy, void,   struct rtos_mutex *task)
svccall(10, rtos_mutex_lock,    void,   struct rtos_mutex *task)
svccall(11, rtos_mutex_trylock, bool,   struct rtos_mutex *task)
svccall(12, rtos_mutex_unlock,  void,   struct rtos_mutex *task)
svccall(13, rtos_cond_create,   void,   struct rtos_cond *cond)
svccall(14, rtos_cond_destroy,  void,   struct rtos_cond *cond)
svccall(15, rtos_cond_wait,     void,   struct rtos_cond *cond,
                                        struct rtos_mutex *mutex)
svccall(16, rtos_cond_signal,   void,   struct rtos_cond *cond)
svccall(17, rtos_cond_broadcast,void,   struct rtos_cond *cond)

void rtos_task_resume_from_isr(rtos_task_t *task)
{
    cm4_disable_irq();
    prv_task_resume(task);
    cm4_enable_irq();
}
