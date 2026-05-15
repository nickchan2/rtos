#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();

    static auto mutex = rtos::Mutex();

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        // mutex.lock();
        // mutex.unlock();
        rtos_test::pass();
    });

    rtos::start();
}
