#include "rtos_test.hh"

namespace {

void task_function(void *task_ptr) {
    EXPECT(rtos::task::self() == task_ptr);
    rtos_test::pass();
}

} // namespace

int main() {
    rtos_test::setup();
    rtos_test::TaskWithStack<> task0(1, false, &task0, task_function);
    rtos_test::TaskWithStack<> task1(1, false, &task1, task_function);
    rtos::start();
}
