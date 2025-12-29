#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto suspended0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(2);
        rtos::task::suspend();
        rtos_test::checkpoint(7);
        rtos_test::pass();
    });

    static auto suspended1 = rtos_test::TaskWithStack(1, false, []{
        rtos_test::checkpoint(1);
        rtos::task::suspend();
        rtos_test::checkpoint(6);
    });

    [[maybe_unused]] static auto resumer = rtos_test::TaskWithStack(0, false, []{
        // Ensure the suspended task is suspended before continuing
        rtos::task::yield();
        rtos_test::checkpoint(3);
        rtos::task::sleep(100);
        rtos_test::checkpoint(4);
        rtos::task::resume(suspended0);
        rtos_test::checkpoint(5);
        rtos::task::resume(suspended1);
    });

    rtos::start();
}
