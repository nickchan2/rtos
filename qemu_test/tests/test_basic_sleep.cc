#include "rtos_test.hh"

int main() {
    quick_setup();
    Task task(1, []{
        for (int i = 0; i < 10; ++i) {
            const int time_before_sleep = HAL_GetTick();
            rtos_task_sleep(5);
            const int elapsed = HAL_GetTick() - time_before_sleep;
            EXPECT(elapsed == 5 || elapsed == 6);
        }
        test_passed();
    });
    rtos_start();
}
