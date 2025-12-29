#include "rtos_test.hh"

auto main() -> int {
    static volatile bool task0_entered = false;
    static volatile bool task1_entered = false;

    rtos_test::setup();
    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(2, false, []{
        task0_entered = true;
    });
    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(2, false, []{
        task1_entered = true;
    });
    [[maybe_unused]] static auto pass_task = rtos_test::TaskWithStack(1, false, [] {
        EXPECT(task0_entered);
        EXPECT(task1_entered);
        rtos_test::pass();
    });
    rtos::start();
}
