#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

namespace {

std::optional<rtos::Mutex> mutex;
std::optional<rtos_test::TaskWithStack<>> task_0;
std::optional<rtos_test::TaskWithStack<>> task_1;

} // namespace

int main() {
    rtos_test::setup();

    mutex.emplace(0);

    task_0.emplace(0, false, []{
        rtos_test::checkpoint(1);
        mutex->lock();
        rtos_test::checkpoint(2);
        
        // Sleep while having the lock
        rtos::task::sleep(rtos::ticks_per_slice * 2);

        mutex->unlock();
    });

    task_1.emplace(0, false, []{
        // Ensure that task0 gets the lock first
        rtos::task::yield();
        rtos_test::checkpoint(3);

        const int time_before = HAL_GetTick();
        mutex->lock();
        rtos_test::checkpoint(4);
        const int elapsed = HAL_GetTick() - time_before;
        EXPECT(elapsed >= static_cast<int>(rtos::ticks_per_slice) * 2 - 1);
        rtos_test::pass();
    });

    rtos::start();
}
