#include "rtos_test.hh"

int main() {
    quick_setup();

    Task low_priority_task(1, []{
        FAIL("Low priority task should not run");
    });

    Task high_priority_task(2, []{
        for (int i = 0; i < 100; ++i) {
            HAL_Delay(1);
            rtos_task_yield();
        }
        test_passed();
    });

    rtos_start();
}
