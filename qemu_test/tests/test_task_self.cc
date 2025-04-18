#include "rtos_test.hh"

static void task_function(void *task_ptr) {
    EXPECT(rtos::task::self() == task_ptr);
    rtos_test::pass();
}

int main() {
    rtos_test::setup();
    rtos::TaskWithStack<> task0(1, &task0, task_function);
    rtos::TaskWithStack<> task1(1, &task1, task_function);
    rtos::start();
}
