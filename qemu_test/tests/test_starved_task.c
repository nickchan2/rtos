#include "rtos.h"
#include "test_utils.h"

#include <assert.h>
#include <stdbool.h>

static void high_priority_task_function(void *)
{
    for (int i = 0; i < 100; ++i) {
        HAL_Delay(1);
        rtos_task_yield();
    }
    test_passed();
}

static void low_priority_task_function(void *)
{
    assert(false);
}

int main(void)
{
    quick_setup();

    struct rtos_task low_priority_task;
    struct rtos_task high_priority_task;
    stack_512_t low_priority_task_stack;
    stack_512_t high_priority_task_stack;
    rtos_task_create(&low_priority_task, &(struct rtos_task_settings){
        .function = low_priority_task_function,
        .task_arg = NULL,
        .stack_low = &low_priority_task_stack,
        .stack_size = sizeof(low_priority_task_stack),
        .priority = 1,
    });
    rtos_task_create(&high_priority_task, &(struct rtos_task_settings){
        .function = high_priority_task_function,
        .task_arg = NULL,
        .stack_low = &high_priority_task_stack,
        .stack_size = sizeof(high_priority_task_stack),
        .priority = 2,
    });
    rtos_start();
}
