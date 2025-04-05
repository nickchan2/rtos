#include "rtos_test.hh"

static volatile int counter = 0;

static void task_function() {
    int last = counter;
    while (true) {
        ++counter;
        if (counter > 100) {
            test_passed();
        }
        last = counter;
        rtos_task_yield();
        EXPECT(counter == last + 1);
    }
}

int main() {
    quick_setup();

    Task task0(1, task_function);
    Task task1(1, task_function);

    rtos_start();
}
