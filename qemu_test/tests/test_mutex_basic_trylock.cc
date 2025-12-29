#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto mutex = rtos::Mutex();

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(1);
        mutex.lock();
        rtos_test::checkpoint(2);
        rtos::task::sleep(5);
        rtos_test::checkpoint(5);
        mutex.unlock();
        rtos_test::checkpoint(6);
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(0, false, []{
        // Ensure that task0 gets the lock first
        rtos::task::yield();
        rtos_test::checkpoint(3);
        bool got_lock = mutex.trylock();
        EXPECT(!got_lock);
        rtos_test::checkpoint(4);
        rtos::task::sleep(10);
        rtos_test::checkpoint(7);
        got_lock = mutex.trylock();
        EXPECT(got_lock);
        rtos_test::pass();
    });

    rtos::start();
}
