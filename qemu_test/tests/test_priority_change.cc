#include "rtos.hh"
#include "rtos_test.hh"
#include "stm32f4xx_hal.h"

#include <optional>

static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();

    mutex.emplace();

    rtos_test::TaskWithStack task_high_pri(2, false, []{
        CHECKPOINT(1);

        // Ensure the low priority task gets the mutex
        rtos::task::sleep(10);

        // After waking up, the low priority task should've got the mutex
        const bool locked = mutex->trylock();
        EXPECT(!locked);

        mutex->lock();
        CHECKPOINT(6);

        rtos_test::pass();
    });

    rtos_test::TaskWithStack task_med_pri(1, false, []{
        CHECKPOINT(2);

        // Ensure the low priority task gets the mutex
        rtos::task::sleep(10);

        FAIL("Task should never run after sleeping");
    });

    rtos_test::TaskWithStack task_low_pri(0, false, []{
        CHECKPOINT(3);

        const bool locked = mutex->trylock();
        EXPECT(locked);
        CHECKPOINT(4);
        HAL_Delay(100);
        CHECKPOINT(5);

        // Releases mutex and returns to low priority
        mutex->unlock();

        FAIL("Task should be preempted");
    });

    rtos::start();
}
