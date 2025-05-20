#include "rtos.hh"
#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack task0(0, false, []{
        rtos::task::sleep(20);
        CHECKPOINT(2);
    });

    rtos_test::TaskWithStack task1(0, false, []{
        rtos::task::sleep(30);
        CHECKPOINT(4);
    });

    rtos_test::TaskWithStack task2(0, false, []{
        rtos::task::sleep(25);
        CHECKPOINT(3);
    });

    rtos_test::TaskWithStack task3(0, false, []{
        rtos::task::sleep(10);
        CHECKPOINT(1);
    });

    rtos_test::TaskWithStack task4(0, false, []{
        rtos::task::sleep(35);
        CHECKPOINT(5);
        rtos_test::pass();
    });

    rtos::start();
}
