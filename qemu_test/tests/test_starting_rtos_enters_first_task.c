#include "rtos.h"
#include "test_utils.h"

#include <stddef.h>

static void task_function(void *) {
    test_passed();
}

int main(void) {
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
