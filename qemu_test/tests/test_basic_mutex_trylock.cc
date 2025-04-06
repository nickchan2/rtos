#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();

    mutex.emplace();

    rtos::TaskWithStack task0(1, []{
        CHECKPOINT(1);
        mutex->lock();
        CHECKPOINT(2);
        rtos::Task::sleep(5);
        CHECKPOINT(5);
        mutex->unlock();
        CHECKPOINT(6);
    });

    rtos::TaskWithStack task1(1, []{
        // Ensure that task0 gets the lock first
        rtos::Task::yield();
        CHECKPOINT(3);
        bool got_lock = mutex->trylock();
        EXPECT(!got_lock);
        CHECKPOINT(4);
        rtos::Task::sleep(10);
        CHECKPOINT(7);
        got_lock = mutex->trylock();
        EXPECT(got_lock);
        rtos_test::pass();
    });

    rtos::start();
}
