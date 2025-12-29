#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos::task::sleep(20);
        rtos_test::checkpoint(2);
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(0, false, []{
        rtos::task::sleep(30);
        rtos_test::checkpoint(4);
    });

    [[maybe_unused]] static auto task2 = rtos_test::TaskWithStack(0, false, []{
        rtos::task::sleep(25);
        rtos_test::checkpoint(3);
    });

    [[maybe_unused]] static auto task3 = rtos_test::TaskWithStack(0, false, []{
        rtos::task::sleep(10);
        rtos_test::checkpoint(1);
    });

    [[maybe_unused]] static auto task4 = rtos_test::TaskWithStack(0, false, []{
        rtos::task::sleep(35);
        rtos_test::checkpoint(5);
        rtos_test::pass();
    });

    rtos::start();
}
