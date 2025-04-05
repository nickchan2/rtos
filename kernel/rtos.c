#include "rtos.h"

#include <assert.h> // TODO: Add internal assertion instead
#include <stdbool.h>
#include <stddef.h>

struct rtos_state {
    // Accessed from assembly begin
    struct rtos_task    *curr_task;
    // Accessed from assembly end
    bool                is_started;
    bool                is_preempting; // TODO: Is there a better alternative?
    size_t              tick_count;
    struct rtos_dll     ready_task_q[RTOS_NUM_PRIORITY_LEVELS];
    struct rtos_dll     sleeping_tasks;
    struct rtos_task    idle_task;
    char                idle_task_stack[256] __attribute__((aligned(8)));
};

// All of the kernel's state is stored here
static struct rtos_state state = {0};

#if RTOS_ENABLE_USAGE_ASSERT

#define usage_assert(cond, msg) \
    if (!(cond)) rtos_failed_assert(#cond, __LINE__, msg)

__attribute((noinline)) static void rtos_failed_assert(const char *cond,
                                                       int line,
                                                       const char *msg)
{
    asm volatile("cpsid i");
    while (true) {}
}

#else // #if RTOS_ENABLE_USAGE_ASSERT

#define usage_assert(cond, msg)

#endif // #if RTOS_ENABLE_USAGE_ASSERT

static void prv_task_create(struct rtos_task *, const struct rtos_task_settings *);

static inline bool dll_is_empty(const struct rtos_dll *dll)
{
    return dll->head == NULL;
}

static struct rtos_task *dll_pop_front(struct rtos_dll *dll)
{
    assert(dll->head != NULL);
    struct rtos_task *const popped = dll->head;
    if (popped->next == NULL) {
        dll->tail = NULL;
    } else {
        popped->next->prev = NULL;
    }
    dll->head = popped->next;
    popped->prev = NULL;
    popped->next = NULL;
    return popped;
}

static void dll_push_front(struct rtos_dll *dll, struct rtos_task *task)
{
    if (dll->head == NULL) {
        dll->head = task;
        dll->tail = task;
        task->prev = NULL;
        task->next = NULL;
    } else {
        dll->head->prev = task;
        task->next = dll->head;
        dll->head = task;
        task->prev = NULL;
    }
}

static void dll_push_back(struct rtos_dll *dll, struct rtos_task *task)
{
    if (dll->head == NULL) {
        dll->head = task;
        dll->tail = task;
        task->prev = NULL;
        task->next = NULL;
    } else {
        dll->tail->next = task;
        task->prev = dll->tail;
        task->next = NULL;
        dll->tail = task;
    }
}

static void dll_insert_ascending(struct rtos_dll *dll, struct rtos_task *task)
{
    struct rtos_task *ptr = dll->head;
    while (ptr != NULL && ptr->wake_time < task->wake_time) {
        ptr = ptr->next;
    }
    if (ptr == NULL) {
        dll_push_back(dll, task);
    } else {
        if (ptr->prev == NULL) {
            dll->head = task;
        } else {
            ptr->prev->next = task;
        }
        task->prev = ptr->prev;
        task->next = ptr;
        ptr->prev = task;
    }
}

static inline void pend_context_switch(void)
{
    // Sent PendSV bit in the ICSR
    static volatile size_t *const ICSR = (volatile size_t *)0xE000ED04U;
    *ICSR |= 1 << 28;
}

// Instead directly entering the task function, a stub function is used which
// calls the task function. This ensures that a task always exits properly.
static void stub(void *arg, rtos_task_func_t task_func)
{
    task_func(arg);
    rtos_task_exit();
    assert(false);
}

// Context switch stack frame:
// See 2-27
// 
// ----- Saved on exception entry only if using FP
// FPSCR
// S15
// ...
// S0
//
// ----- Always saved on exception entry
// xPSR
// PC
// LR
// R12
// R3
// ...
// R0
// 
// ----- Always saved by context switch handler
// R11
// ...
// R4
//
// ----- Saved by the context switch handler using FP
// S31
// ...
// S16
//
// ----- Always saved by context switch handler
// EXC_RETURN (indicates if FP is used)

static size_t *create_switch_frame(const struct rtos_task_settings *settings)
{
    size_t *stack_top = settings->stack_low + settings->stack_size;
    const rtos_task_func_t task_func = settings->function;
    const void *task_arg = settings->task_arg;

    usage_assert(((size_t)stack_top & 0b111) == 0,
                 "Stack must be 8-byte aligned");

    *(--stack_top) = 1 << 24;           // PSR (indicate thumb mode)
    *(--stack_top) = (size_t)stub;      // PC

    for (int i = 0; i < 4; ++i) {
        *(--stack_top) = 0;             // LR, R12, R3, R2
    }

    *(--stack_top) = (size_t)task_func; // R1 (Arg 1 for stub)
    *(--stack_top) = (size_t)task_arg;  // R0 (Arg 0 for stub)

    for (int i = 0; i < 8; ++i) {
        *(--stack_top) = 0;             // R11 - R4
    }

    // See 2-28
    *(--stack_top) = 0xFFFFFFFDU;   // EXC_RETURN (thread mode, PSP, non-FP)

    return stack_top;
}

static void idle_task(void *args)
{
    while (true) {
        asm volatile("wfi");
    }
}

static void task_init(struct rtos_task *task,
                      const struct rtos_task_settings *settings)
{
    task->stack_low     = settings->stack_low;
    task->switch_frame  = create_switch_frame(settings);
    task->priority      = settings->priority;
    task->slice_left    = RTOS_TICKS_PER_SLICE;
    task->wake_time     = 0;
    task->state         = RTOS_TASKSTATE_READY;
    task->prev          = NULL;
    task->next          = NULL;
}

/* ----------------------------------------------------------------------------
 * System call implementations
 * ------------------------------------------------------------------------- */

static void prv_start(void)
{
    usage_assert(state.is_started == false, "RTOS already started");
    state.is_started = true;

    static volatile size_t *const FPCSR = (volatile size_t *)0xE000EF34U;
    *FPCSR |= (1 << 31) | (1 << 30); // ASPEN | LSPEN

    prv_task_create(&state.idle_task, &(struct rtos_task_settings){
        .function = idle_task,
        .task_arg = NULL,
        .stack_low = state.idle_task_stack,
        .stack_size = sizeof(state.idle_task_stack),
        .priority = 0,
    });

    pend_context_switch();
}

static void prv_task_create(struct rtos_task *task,
                            const struct rtos_task_settings *settings)
{
    usage_assert(settings->function != NULL, "Passed NULL task function");
    usage_assert((size_t)settings->stack_low % 8 == 0,
                 "Stack low address must be 8-byte aligned");
    usage_assert(settings->stack_size >= 256,
                 "Stack size must be at least 256 bytes");
    usage_assert(settings->stack_size % 8 == 0,
                 "Stack size must be multiple of 8");
    // FIXME: Can be zero if the task is the idle task.
    // usage_assert(priority > 0 && priority <= RTOS_MAX_TASK_PRIORITY,
    //              "Task priority must be in range [1,RTOS_MAX_TASK_PRIORITY]");

    task_init(task, settings);

    const size_t priority = settings->priority;

    dll_push_back(&state.ready_task_q[priority], task);

    if (state.curr_task != NULL && state.curr_task->priority < priority) {
        // The priority of the new task is higher than the priority of the task
        // that created it. The new task preempts the current task. */
        state.curr_task->state = RTOS_TASKSTATE_READY;
        dll_push_front(&state.ready_task_q[state.curr_task->priority],
                       state.curr_task);
        pend_context_switch();
    }
}

static struct rtos_task *prv_task_self(void)
{
    usage_assert(state.is_started, "RTOS must be started before calling");
    return state.curr_task;
}

static void prv_task_exit(void)
{
    usage_assert(state.is_started, "RTOS must be started before calling");

    // Setting the current task to NULL indicates that the context switch
    // handler shouldn't save the context of the exited task.
    state.curr_task = NULL;
    pend_context_switch();
}

static void prv_task_yield(void)
{
    usage_assert(state.is_started, "RTOS must be started before calling");
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
    usage_assert(state.is_started, "RTOS must be started before calling");
    usage_assert(ticks > 0, "Number of ticks must be greater than zero");

    state.curr_task->wake_time = state.tick_count + ticks;
    state.curr_task->state = RTOS_TASKSTATE_SLEEPING;
    dll_insert_ascending(&state.sleeping_tasks, state.curr_task);

    pend_context_switch();
}

static void prv_task_suspend(void)
{
    usage_assert(state.is_started, "RTOS must be started before calling");
    state.curr_task->state = RTOS_TASKSTATE_SUSPENDED;
    pend_context_switch();
}

static void prv_task_resume(struct rtos_task *task)
{
    usage_assert(task != NULL, "Passed NULL task handle");
    if (!state.is_started || task->state != RTOS_TASKSTATE_SUSPENDED) {
        return;
    }
    task->state = RTOS_TASKSTATE_READY;
    dll_push_back(&state.ready_task_q[task->priority], task);
    if (task->priority > state.curr_task->priority && !state.is_preempting) {
        state.is_preempting = true;
        state.curr_task->state = RTOS_TASKSTATE_READY;
        dll_push_front(&state.ready_task_q[state.curr_task->priority],
                       state.curr_task);
        pend_context_switch();
    }
}

static void prv_mutex_create(struct rtos_mutex *mutex)
{
    mutex->owner = NULL;
    mutex->blocked.head = NULL;
    mutex->blocked.tail = NULL;
}

static void prv_mutex_destroy(struct rtos_mutex *mutex)
{
    usage_assert(mutex != NULL, "Passed NULL mutex handle");
    usage_assert(mutex->owner == NULL,
                 "Destroying mutex that tasks are still waiting on");
    assert(dll_is_empty(&mutex->blocked));
}

static void prv_mutex_lock(struct rtos_mutex *mutex)
{
    usage_assert(mutex != NULL, "Passed NULL mutex handle");
    usage_assert(state.is_started, "RTOS must be started before calling");

    usage_assert(mutex->owner != state.curr_task,
                 "Attempt to double lock mutex");
    if (mutex->owner != NULL) {
        state.curr_task->state = RTOS_TASKSTATE_WAIT_MUTEX;
        dll_push_back(&mutex->blocked, state.curr_task);
        pend_context_switch();
    } else {
        mutex->owner = state.curr_task;
    }
}

static bool prv_mutex_trylock(struct rtos_mutex *mutex)
{
    usage_assert(mutex != NULL, "Passed NULL mutex handle");
    usage_assert(state.is_started, "RTOS must be started before calling");
    if (mutex->owner != NULL) {
        return false;
    }
    mutex->owner = state.curr_task;
    return true;
}

static void prv_mutex_unlock(struct rtos_mutex *mutex)
{
    usage_assert(mutex != NULL, "Passed NULL mutex handle");
    usage_assert(state.is_started, "RTOS must be started before calling");
    usage_assert(mutex->owner == state.curr_task,
                 "Task other than owner tried to unlock mutex");

    if (dll_is_empty(&mutex->blocked)) {
        // No tasks were waiting to aquire so the mutex becomes unlocked.
        mutex->owner = NULL;
    } else {
        // Pass the mutex to the unblocked task.
        struct rtos_task *const unblocked = dll_pop_front(&mutex->blocked);
        unblocked->state = RTOS_TASKSTATE_READY;
        mutex->owner = unblocked;
        dll_push_back(&state.ready_task_q[unblocked->priority],
                      unblocked);

        if (unblocked->priority < state.curr_task->priority) {
            state.curr_task->state = RTOS_TASKSTATE_READY;
            dll_push_front(&state.ready_task_q[state.curr_task->priority],
                           state.curr_task);
            pend_context_switch();
        }
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
    usage_assert(cond != NULL, "Passed NULL cond handle");
    usage_assert(dll_is_empty(&cond->waiting),
                 "Tried to destroy cond while there were tasks waiting on it");
    assert(cond->mutex == NULL);
}

static void prv_cond_wait(struct rtos_cond *cond, struct rtos_mutex *mutex)
{
    usage_assert(cond != NULL, "Passed NULL cond handle");
    usage_assert(mutex != NULL, "Passed NULL mutex handle");
    usage_assert(state.is_started, "RTOS must be started before calling");
    usage_assert(cond->mutex == NULL || cond->mutex == mutex, 
                 "The cv is already associated with another mutex");

    prv_mutex_unlock(mutex);

    cond->mutex = mutex;
    state.curr_task->state = RTOS_TASKSTATE_WAIT_COND;
    dll_push_back(&cond->waiting, state.curr_task);
    pend_context_switch();
}

static void cond_wake_task(struct rtos_cond *cond)
{
    struct rtos_task *const waken = dll_pop_front(&cond->waiting);
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
    usage_assert(cond != NULL, "Passed NULL cond handle");
    usage_assert(state.is_started, "RTOS must be started before calling");
    if (!dll_is_empty(&cond->waiting)) {
        cond_wake_task(cond);
    }
    if (dll_is_empty(&cond->waiting)) {
        cond->mutex = NULL;
    }
}

static void prv_cond_broadcast(struct rtos_cond *cond)
{
    usage_assert(cond != NULL, "Passed NULL cond handle");
    usage_assert(state.is_started, "RTOS must be started before calling");
    while (!dll_is_empty(&cond->waiting)) {
        cond_wake_task(cond);
    }
    cond->mutex = NULL;
}

/* ----------------------------------------------------------------------------
 * Interrupt handlers
 * ------------------------------------------------------------------------- */

__attribute((used)) static void svc_handler_main(size_t *stack)
{
    // The SVC number is encoded in the low byte of the SVC instruction. To
    // access it, the PC saved on the stack during exception entry is used.
    const int svc_num = ((unsigned char *)stack[6])[-2];
    size_t rv = 0;
    switch (svc_num) {
        case 0:
            prv_start();
            break;
        case 1:
            prv_task_create((struct rtos_task *)stack[0],
                                            (const struct rtos_task_settings *)stack[1]);
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
            prv_task_sleep(stack[0]);
            break;
        case 6:
            prv_task_suspend();
            break;
        case 7:
            prv_task_resume((void *)stack[0]);
            break;
        case 8:
            prv_mutex_create((void *)stack[0]);
            break;
        case 9:
            prv_mutex_destroy((void *)stack[0]);
            break;
        case 10:
            prv_mutex_lock((void *)stack[0]);
            break;
        case 11:
            rv = prv_mutex_trylock((void *)stack[0]);
            break;
        case 12:
            prv_mutex_unlock((void *)stack[0]);
            break;
        case 13:
            prv_cond_create((void *)stack[0]);
            break;
        case 14:
            prv_cond_destroy((void *)stack[0]);
            break;
        case 15:
            prv_cond_wait((void *)stack[0], (void *)stack[1]);
            break;
        case 16:
            prv_cond_signal((void *)stack[0]);
            break;
        case 17:
            prv_cond_broadcast((void *)stack[0]);
            break;
        default:
            assert(false);
            break;
    }

    // Store the return value in the part of the stack that gets popped to R0
    // when the handler returns.
    stack[0] = rv;
}

__attribute((naked)) void SVC_Handler(void)
{
    asm volatile(
    "   tst     lr, #4              \n"
    "   ite     eq                  \n"
    "   mrseq   r0, msp             \n"
    "   mrsne   r0, psp             \n"
    "   b       svc_handler_main    \n"
    );
}

// Returns a pointer to the next task's switch frame.
__attribute((used)) static size_t *choose_next_task(size_t *old_switch_frame)
{
    if (state.curr_task != NULL) {
        usage_assert((size_t)old_switch_frame >=
                         (size_t)state.curr_task->stack_low,
                     "Task stack overflow");
        assert(state.curr_task->state != RTOS_TASKSTATE_RUNNING);
        state.curr_task->switch_frame = old_switch_frame;
    }

    // Choose the highest priority task that's ready to run next.
    for (int i = RTOS_MAX_TASK_PRIORITY; i > -1; --i) {
        if (!dll_is_empty(&state.ready_task_q[i])) {
            state.curr_task = dll_pop_front(&state.ready_task_q[i]);
            break;
        }
        assert(i != 0);
    }

    assert(state.curr_task->state == RTOS_TASKSTATE_READY);
    state.curr_task->state = RTOS_TASKSTATE_RUNNING;
    state.is_preempting = false;
    return state.curr_task->switch_frame;
}

// Interrupts are disabled during this to ensure an atomic context switch. This
// is needed because PendSV has the lowest priority and otherwise, another
// interrupt could preempt this handler and call a kernel function while the
// kernel state is invalid.
__attribute__((naked)) void PendSV_Handler(void)
{
    asm volatile(
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
        struct rtos_task *const waken = dll_pop_front(&state.sleeping_tasks);
        assert(waken->state == RTOS_TASKSTATE_SLEEPING);
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
    __attribute((naked)) ret name(__VA_ARGS__) {    \
        asm volatile(                               \
        "   svc    "#num"   \n"                     \
        "   bx      lr      \n"                     \
        );                                          \
    }

svccall(0,  rtos_start,         void,   void)
svccall(1,  rtos_task_create,   void,   struct rtos_task *task,
                                        const struct rtos_task_settings *settings)
svccall(2,  rtos_task_self,     struct rtos_task *, void)
svccall(3,  rtos_task_exit,     void,   void)
svccall(4,  rtos_task_yield,    void,   void)
svccall(5,  rtos_task_sleep,    void,   size_t ticks)
svccall(6,  rtos_task_suspend,  void,   void)
svccall(7,  rtos_task_resume,   void,   struct rtos_task *task)
svccall(8,  rtos_mutex_create,  void,   struct rtos_mutex *mutex)
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

void rtos_task_resume_from_isr(struct rtos_task *task)
{
    asm volatile("cpsid i");
    prv_task_resume(task);
    asm volatile("cpsie i");
}
