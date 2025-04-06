#include "rtos.h"
#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();

    mutex.emplace();

    rtos::Mutex mutex2;

    rtos::TaskWithStack task0(1, []{
        CHECKPOINT(1);
        mutex->lock();
        CHECKPOINT(2);
        
        // Sleep while having the lock
        rtos::Task::sleep(RTOS_TICKS_PER_SLICE * 2);

        mutex->unlock();
        CHECKPOINT(4);
    });

    rtos::TaskWithStack task1(1, []{
        // Ensure that task0 gets the lock first
        rtos::Task::yield();
        CHECKPOINT(3);

        const int time_before = HAL_GetTick();
        mutex->lock();
        CHECKPOINT(5);
        const int elapsed = HAL_GetTick() - time_before;
        EXPECT(elapsed >= (RTOS_TICKS_PER_SLICE - 1) * 2);
        rtos_test::pass();
    });

    rtos::start();
}
