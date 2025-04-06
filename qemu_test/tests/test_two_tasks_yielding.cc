#include "rtos_test.hh"

static volatile int counter = 0;

static void task_function() {
    int last = counter;
    while (true) {
        ++counter;
        if (counter > 100) {
            rtos_test::pass();
        }
        last = counter;
        rtos::Task::yield();
        EXPECT(counter == last + 1);
    }
}

int main() {
    rtos_test::setup();

    rtos::TaskWithStack task0(1, task_function);
    rtos::TaskWithStack task1(1, task_function);

    rtos::start();
}
