#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack task0(0, false, []{
        rtos_test::checkpoint(2);
        rtos::task::sleep(10);
        rtos_test::checkpoint(4);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        rtos_test::checkpoint(1);
        rtos::task::sleep(10);
        rtos_test::checkpoint(3);
    });

    rtos::start();
}
