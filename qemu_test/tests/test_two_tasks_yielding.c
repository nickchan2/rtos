#include "rtos.h"
#include "test_utils.h"

#include <stdbool.h>

static volatile int counter = 0;

static void task(void *)
{
    int last = counter;
    while (true) {
        ++counter;
        if (counter > 100) {
            test_passed();
        }
        last = counter;
        rtos_task_yield();
        assert(counter == last + 1);
    }
}

int main(void)
{
    quick_setup();

    stack_512_t stacks[2];
    struct rtos_task tasks[2];

    for (int i = 0; i < 2; ++i) {
        rtos_task_create(&tasks[i], &(struct rtos_task_settings){
            .function = task,
            .task_arg = NULL,
            .stack_low = &stacks[i],
            .stack_size = sizeof(stacks[i]),
            .priority = 1,
        });
    }

    rtos_start();
}
