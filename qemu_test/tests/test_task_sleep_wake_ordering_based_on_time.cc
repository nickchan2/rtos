#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos::Task::sleep_for(20);
        rtos_test::checkpoint(2);
        rtos::Task::suspend();
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(0, false, []{
        rtos::Task::sleep_for(30);
        rtos_test::checkpoint(4);
        rtos::Task::suspend();
    });

    [[maybe_unused]] static auto task2 = rtos_test::TaskWithStack(0, false, []{
        rtos::Task::sleep_for(25);
        rtos_test::checkpoint(3);
        rtos::Task::suspend();
    });

    [[maybe_unused]] static auto task3 = rtos_test::TaskWithStack(0, false, []{
        rtos::Task::sleep_for(10);
        rtos_test::checkpoint(1);
        rtos::Task::suspend();
    });

    [[maybe_unused]] static auto task4 = rtos_test::TaskWithStack(0, false, []{
        rtos::Task::sleep_for(35);
        rtos_test::checkpoint(5);
        rtos_test::pass();
    });

    rtos::start();
}
