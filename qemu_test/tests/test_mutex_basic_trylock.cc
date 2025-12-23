#include "rtos_test.hh"

#include <optional>

namespace {

std::optional<rtos::Mutex> mutex;

} // namespace

int main() {
    rtos_test::setup();

    mutex.emplace();

    rtos_test::TaskWithStack task0(0, false, []{
        rtos_test::checkpoint(1);
        mutex->lock();
        rtos_test::checkpoint(2);
        rtos::task::sleep(5);
        rtos_test::checkpoint(5);
        mutex->unlock();
        rtos_test::checkpoint(6);
    });

    rtos_test::TaskWithStack task1(0, false, []{
        // Ensure that task0 gets the lock first
        rtos::task::yield();
        rtos_test::checkpoint(3);
        bool got_lock = mutex->trylock();
        EXPECT(!got_lock);
        rtos_test::checkpoint(4);
        rtos::task::sleep(10);
        rtos_test::checkpoint(7);
        got_lock = mutex->trylock();
        EXPECT(got_lock);
        rtos_test::pass();
    });

    rtos::start();
}
