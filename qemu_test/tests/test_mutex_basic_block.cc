#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto mutex = rtos::Mutex(0);

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(1);
        mutex.lock();
        rtos_test::checkpoint(2);
        
        // Sleep while having the lock
        rtos::task::sleep(rtos::ticks_per_slice * 2);

        mutex.unlock();
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(0, false, []{
        // Ensure that task0 gets the lock first
        rtos::task::yield();
        rtos_test::checkpoint(3);

        const uint32_t elapsed = rtos_test::measure_ticks([]{
            mutex.lock();
        });
        EXPECT(elapsed >= static_cast<int>(rtos::ticks_per_slice) * 2 - 1);
        rtos_test::checkpoint(4);
        rtos_test::pass();
    });

    rtos::start();
}
