#include "rtos.h"
#include "test_utils.h"

static void task_function(void *task_ptr) {
    assert(rtos_task_self() == task_ptr);
    test_passed();
}

int main(void) {
    quick_setup();
    stack_512_t stack;
    static struct rtos_task task;
    rtos_task_create(&task, &(struct rtos_task_settings){
        .function = task_function,
        .task_arg = &task,
        .stack_low = &stack,
        .stack_size = sizeof(stack),
        .priority = 1,
    });
    rtos_start();
}
