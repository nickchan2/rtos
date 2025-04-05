#include "rtos.h"
#include "test_utils.h"

static const int waiter_count = 4;

static volatile int counter = 0;
static struct rtos_cond cond;
static struct rtos_mutex mutex;

static void signaler_function(void *)
{
    // Ensure all waiters are waiting before signaling
    rtos_task_sleep(100);

    rtos_cond_broadcast(&cond);
    rtos_task_sleep(100);
    rtos_mutex_lock(&mutex);
    assert(counter == waiter_count);
    rtos_mutex_unlock(&mutex);
    test_passed();
}

static void waiter_function(void *)
{
    rtos_mutex_lock(&mutex);
    rtos_cond_wait(&cond, &mutex);
    ++counter;
    rtos_mutex_unlock(&mutex);
}

int main(void)
{
    quick_setup();
    rtos_cond_create(&cond);
    rtos_mutex_create(&mutex);
    struct rtos_task waiter_tasks[waiter_count];
    stack_512_t waiter_stacks[waiter_count];
    for (int i = 0; i < waiter_count; ++i) {
        rtos_task_create(&waiter_tasks[i], &(struct rtos_task_settings){
            .function = waiter_function,
            .task_arg = NULL,
            .stack_low = &waiter_stacks[i],
            .stack_size = sizeof(waiter_stacks[i]),
            .priority = 1,
        });
    }
    struct rtos_task signaler_task;
    stack_512_t signaler_stack;
    rtos_task_create(&signaler_task, &(struct rtos_task_settings){
        .function = signaler_function,
        .task_arg = NULL,
        .stack_low = &signaler_stack,
        .stack_size = sizeof(signaler_stack),
        .priority = 1,
    });

    rtos_start();
}
