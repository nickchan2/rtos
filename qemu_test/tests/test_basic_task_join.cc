#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(3);
        rtos::task::sleep(rtos::ticks_per_slice * 2);
        rtos_test::checkpoint(4);
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(1, false, []{
        rtos_test::checkpoint(2);
        rtos::task::join(task0);
        rtos_test::checkpoint(6);
        rtos_test::pass();
    });

    [[maybe_unused]] static auto task2 = rtos_test::TaskWithStack(2, false, []{
        rtos_test::checkpoint(1);
        rtos::task::join(task0);
        rtos_test::checkpoint(5);
        rtos_test::pass();
    });

    rtos::start();
}
