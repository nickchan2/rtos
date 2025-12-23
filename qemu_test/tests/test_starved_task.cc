#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack low_priority_task(1, false, []{
        rtos_test::fail("Low priority task should not run");
    });

    rtos_test::TaskWithStack high_priority_task(2, false, []{
        for (int i = 0; i < 100; ++i) {
            HAL_Delay(1);
            rtos::task::yield();
        }
        rtos_test::pass();
    });

    rtos::start();
}
