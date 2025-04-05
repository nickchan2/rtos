#include "rtos_test.hh"

static void task_function(void *task_ptr) {
    assert(rtos_task_self() == task_ptr);
    test_passed();
}

int main(void) {
    quick_setup();
    Task task0(1, &task0.task, task_function);
    Task task1(1, &task1.task, task_function);
    rtos_start();
}
