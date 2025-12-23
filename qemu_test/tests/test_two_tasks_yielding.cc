#include "rtos_test.hh"

namespace {

volatile int counter = 0;

void task_function() {
    int last = counter;
    while (true) {
        counter = counter + 1;
        if (counter > 100) {
            rtos_test::pass();
        }
        last = counter;
        rtos::task::yield();
        EXPECT(counter == last + 1);
    }
}

} // namespace

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack task0(1, false, task_function);
    rtos_test::TaskWithStack task1(1, false, task_function);

    rtos::start();
}
