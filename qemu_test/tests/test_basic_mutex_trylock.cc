#include "rtos_test.hh"

#include <optional>

static std::optional<Mutex> mutex;

int main() {
    quick_setup();

    mutex.emplace();

    Task task0(1, []{
        CHECKPOINT(1);
        mutex->lock();
        CHECKPOINT(2);
        rtos_task_sleep(5);
        CHECKPOINT(5);
        mutex->unlock();
        CHECKPOINT(6);
    });

    Task task1(1, []{
        // Ensure that task0 gets the lock first
        rtos_task_yield();
        CHECKPOINT(3);
        bool got_lock = mutex->trylock();
        EXPECT(!got_lock);
        CHECKPOINT(4);
        rtos_task_sleep(10);
        CHECKPOINT(7);
        got_lock = mutex->trylock();
        EXPECT(got_lock);
        test_passed();
    });

    rtos_start();
}
