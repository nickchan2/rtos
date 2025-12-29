#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto mutex = rtos::Mutex(1);

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(3);
        mutex.lock();
        rtos_test::checkpoint(8);
        mutex.unlock();
        rtos_test::checkpoint(9);
        rtos_test::pass();
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(1, false, []{
        rtos::task::sleep(10);
        rtos_test::checkpoint(4);
        mutex.lock();
        rtos_test::checkpoint(6);
        mutex.unlock();
        rtos_test::checkpoint(7);
    });

    [[maybe_unused]] static auto task2 = rtos_test::TaskWithStack(1, false, []{
        rtos_test::checkpoint(1);
        mutex.lock();
        rtos_test::checkpoint(2);
        rtos::task::sleep(20);
        rtos_test::checkpoint(5);
        mutex.unlock();
    });

    rtos::start();
}
