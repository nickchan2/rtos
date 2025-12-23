#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

namespace {

std::optional<rtos::Mutex> mutex0;
std::optional<rtos::Mutex> mutex1;
std::optional<rtos_test::TaskWithStack<>> task_high_pri;

} // namespace

int main() {
    rtos_test::setup();

    mutex0.emplace();
    mutex1.emplace();

    task_high_pri.emplace(2, false, []{
        rtos_test::checkpoint(1);
        rtos::task::suspend();
        rtos_test::checkpoint(5);
        rtos::task::yield();
        rtos_test::checkpoint(7);
        rtos::task::yield();
        rtos_test::checkpoint(9);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task_med_pri(1, false, []{
        rtos_test::checkpoint(2);

        // Ensure the low priority task gets the mutex
        rtos::task::sleep(5);

        rtos_test::fail("Task should never run after sleeping");
    });

    rtos_test::TaskWithStack task_low_pri(0, false, []{
        rtos_test::checkpoint(3);

        EXPECT(mutex0->trylock());
        EXPECT(mutex1->trylock());

        rtos_test::checkpoint(4);
    
        rtos::task::resume(&task_high_pri.value());
        rtos::task::yield();

        rtos_test::checkpoint(6);

        // Unlock but stay at high priority due to still holding mutex1
        mutex0->unlock();
        rtos::task::yield();

        rtos_test::checkpoint(8);

        // Unlock and return to low priority
        mutex1->unlock();

        rtos_test::fail("Task should be preempted");
    });

    rtos::start();
}
