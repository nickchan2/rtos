#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack task0(0, false, []{
        CHECKPOINT(2);
        rtos::task::sleep(10);
        CHECKPOINT(4);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        CHECKPOINT(1);
        rtos::task::sleep(10);
        CHECKPOINT(3);
    });

    rtos::start();
}
