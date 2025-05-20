#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();

    mutex.emplace();

    rtos_test::TaskWithStack task0(0, false, []{
        CHECKPOINT(1);
        mutex->lock();
        CHECKPOINT(2);
        rtos::task::sleep(5);
        CHECKPOINT(5);
        mutex->unlock();
        CHECKPOINT(6);
    });

    rtos_test::TaskWithStack task1(0, false, []{
        // Ensure that task0 gets the lock first
        rtos::task::yield();
        CHECKPOINT(3);
        bool got_lock = mutex->trylock();
        EXPECT(!got_lock);
        CHECKPOINT(4);
        rtos::task::sleep(10);
        CHECKPOINT(7);
        got_lock = mutex->trylock();
        EXPECT(got_lock);
        rtos_test::pass();
    });

    rtos::start();
}
