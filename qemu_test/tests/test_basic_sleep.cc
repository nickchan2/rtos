#include "rtos_test.hh"

int main() {
    rtos_test::setup();
    rtos::TaskWithStack task(1, []{
        for (int i = 0; i < 10; ++i) {
            const int time_before_sleep = HAL_GetTick();
            rtos::task::sleep(5);
            const int elapsed = HAL_GetTick() - time_before_sleep;
            EXPECT(elapsed == 5 || elapsed == 6);
        }
        rtos_test::pass();
    });
    rtos::start();
}
