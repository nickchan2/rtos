#include "rtos.h"
#include "test_utils.h"

void task_function(void *)
{
    for (int i = 0; i < 10; ++i) {
        int const time_before_sleep = HAL_GetTick();
        rtos_task_sleep(5);
        int const elapsed = HAL_GetTick() - time_before_sleep;
        assert(elapsed == 5 || elapsed == 6);
    }
    test_passed();
}

int main(void)
{
    quick_setup();
    struct rtos_task task;
    stack_512_t stack;
    rtos_task_create(&task, &(struct rtos_task_settings){
        .function = task_function,
        .task_arg = NULL,
        .stack_low = &stack,
        .stack_size = sizeof(stack),
        .priority = 1,
    });
    rtos_start();
}
