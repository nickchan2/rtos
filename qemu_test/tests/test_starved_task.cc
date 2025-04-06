#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos::TaskWithStack low_priority_task(1, []{
        FAIL("Low priority task should not run");
    });

    rtos::TaskWithStack high_priority_task(2, []{
        for (int i = 0; i < 100; ++i) {
            HAL_Delay(1);
            rtos::Task::yield();
        }
        rtos_test::pass();
    });

    rtos::start();
}
