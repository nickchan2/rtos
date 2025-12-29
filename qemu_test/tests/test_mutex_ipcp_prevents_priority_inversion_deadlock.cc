#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto mutex = rtos::Mutex(1);

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(1, false, []{
        rtos_test::checkpoint(1);
        rtos::task::suspend();
        rtos_test::checkpoint(4);
        mutex.lock();
        rtos_test::checkpoint(6);
        rtos_test::pass();
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(1, false, []{
        rtos::task::sleep(10);
        while (true) {}
    });

    [[maybe_unused]] static auto task2 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(2);
        mutex.lock();
        rtos_test::checkpoint(3);
        rtos::task::resume(task0);
        rtos::task::sleep(20);
        rtos_test::checkpoint(5);
        mutex.unlock();
        rtos_test::fail("Task should be preempted");
    });

    rtos::start();
}
