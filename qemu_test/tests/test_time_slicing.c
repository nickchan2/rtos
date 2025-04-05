#include "rtos.h"
#include "test_utils.h"

static volatile int counter = 0;

static void task_function(void *)
{
    while (true) {
        ++counter;
        const int last_count = counter;
        const int last_time = HAL_GetTick();
        while (counter == last_count) {}
        const int elapsed = HAL_GetTick() - last_time;
        assert(elapsed >= RTOS_TICKS_PER_SLICE * 2 - 1);

        if (counter >= 10) {
            test_passed();
        }
    }
}

int main(void)
{
    quick_setup();
    struct rtos_task tasks[2];
    stack_512_t stacks[2];
    for (int i = 0; i < 2; ++i) {
        rtos_task_create(&tasks[i], &(struct rtos_task_settings){
            .function = task_function,
            .task_arg = NULL,
            .stack_low = &stacks[i],
            .stack_size = sizeof(stacks[i]),
            .priority = 1,
        });
    }
    rtos_start();
}
