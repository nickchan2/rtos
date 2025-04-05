#include "rtos.h"
#include "test_utils.h"

static void resumer_function(void *suspended_task)
{
    // Ensure the suspended task is suspended before continuing
    rtos_task_yield();

    rtos_task_sleep(100);
    rtos_task_resume(suspended_task);
}

static void suspended_function(void *)
{
    const int time_before_suspend = HAL_GetTick();
    rtos_task_suspend();
    int const elapsed = HAL_GetTick() - time_before_suspend;
    assert(elapsed >= 100);
    test_passed();
}

int main(void)
{
    quick_setup();

    struct rtos_task suspended_task;
    stack_512_t suspended_stack;
    rtos_task_create(&suspended_task, &(struct rtos_task_settings){
        .function = suspended_function,
        .task_arg = NULL,
        .stack_low = &suspended_stack,
        .stack_size = sizeof(suspended_stack),
        .priority = 1,
    });

    struct rtos_task resumer_task;
    stack_512_t resumer_stack;
    rtos_task_create(&resumer_task, &(struct rtos_task_settings){
        .function = resumer_function,
        .task_arg = &suspended_task,
        .stack_low = &resumer_stack,
        .stack_size = sizeof(resumer_stack),
        .priority = 1,
    });

    rtos_start();
}
