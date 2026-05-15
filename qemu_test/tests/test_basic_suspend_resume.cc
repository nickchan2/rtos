#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto suspended0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(2);
        rtos::Task::suspend();
        rtos_test::checkpoint(7);
        rtos_test::pass();
    });

    static auto suspended1 = rtos_test::TaskWithStack(1, false, []{
        rtos_test::checkpoint(1);
        rtos::Task::suspend();
        rtos_test::checkpoint(6);
        rtos::Task::suspend();
    });

    [[maybe_unused]] static auto resumer = rtos_test::TaskWithStack(0, false, []{
        // Ensure the suspended task is suspended before continuing
        rtos::Task::yield();
        rtos_test::checkpoint(3);
        rtos::Task::sleep_for(100);
        rtos_test::checkpoint(4);
        rtos::Task::resume(suspended0.task);
        rtos_test::checkpoint(5);
        rtos::Task::resume(suspended1.task);
        rtos::Task::suspend();
    });

    rtos::start();
}
