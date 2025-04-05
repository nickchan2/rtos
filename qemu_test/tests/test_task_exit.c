#include "rtos.h"
#include "test_utils.h"

#include <stddef.h>

static volatile bool task0_entered = false;
static volatile bool task1_entered = false;

static void task0(void *)
{
    task0_entered = true;
}

static void task1(void *)
{
    task1_entered = true;
    rtos_task_exit();
}

static void pass_task(void *)
{
    assert(task0_entered);
    assert(task1_entered);
    test_passed();
}

int main(void)
{
    quick_setup();
    struct rtos_task task[3];
    stack_512_t stacks[3];
    rtos_task_func_t task_funcs[3] = {task0, task1, pass_task};
    for (int i = 0; i < 3; ++i) {
        rtos_task_create(&task[i], &(struct rtos_task_settings){
            .function = task_funcs[i],
            .task_arg = NULL,
            .stack_low = &stacks[i],
            .stack_size = sizeof(stacks[i]),
            .priority = 1,
        });
    }
    rtos_start();
}
