#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    [[maybe_unused]] static auto privileged = rtos_test::TaskWithStack(1, true, []{
        rtos_test::checkpoint(1);
        __disable_irq();
        __enable_irq();
        rtos::Task::suspend();
    });

    [[maybe_unused]] static auto unprivileged = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(2);
        rtos_test::expect_hardfault_to_pass([]{ __disable_irq(); });
    });

    rtos::start();
}
