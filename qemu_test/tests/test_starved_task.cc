#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    [[maybe_unused]] static auto low_priority_task = rtos_test::TaskWithStack(1, false, []{
        rtos_test::fail("Low priority task should not run");
    });

    [[maybe_unused]] static auto high_priority_task = rtos_test::TaskWithStack(2, false, []{
        for (int i = 0; i < 100; ++i) {
            HAL_Delay(1);
            rtos::task::yield();
        }
        rtos_test::pass();
    });

    rtos::start();
}
