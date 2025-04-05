#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();

    mutex.emplace(0);

    rtos_test::TaskWithStack task0(0, false, []{
        CHECKPOINT(1);
        mutex->lock();
        CHECKPOINT(2);
        
        // Sleep while having the lock
        rtos::task::sleep(rtos::ticks_per_slice * 2);

        mutex->unlock();
    });

    rtos_test::TaskWithStack task1(0, false, []{
        // Ensure that task0 gets the lock first
        rtos::task::yield();
        CHECKPOINT(3);

        const int time_before = HAL_GetTick();
        mutex->lock();
        CHECKPOINT(4);
        const int elapsed = HAL_GetTick() - time_before;
        EXPECT(elapsed >= static_cast<int>(rtos::ticks_per_slice) * 2 - 1);
        rtos_test::pass();
    });

    rtos::start();
}
