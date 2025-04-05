#include "rtos_test.hh"

static void task_function(void *arg) {
    rtos_task **task_ptr = static_cast<rtos_task **>(arg);
    EXPECT(rtos_task_self() == *task_ptr);
    test_passed();
}

int main() {
    quick_setup();
    rtos_task *task0_ptr = nullptr;
    rtos_task *task1_ptr = nullptr;
    Task task0(1, &task0_ptr, task_function);
    Task task1(1, &task1_ptr, task_function);
    task0_ptr = &task0.task;
    task1_ptr = &task1.task;
    rtos_start();
}
