#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(2);
        rtos::task::sleep(10);
        rtos_test::checkpoint(4);
        rtos_test::pass();
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(1, false, []{
        rtos_test::checkpoint(1);
        rtos::task::sleep(10);
        rtos_test::checkpoint(3);
    });

    rtos::start();
}
