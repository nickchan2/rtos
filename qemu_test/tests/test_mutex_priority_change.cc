#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mutex> mutex0;
static std::optional<rtos::Mutex> mutex1;
static std::optional<rtos_test::TaskWithStack<>> task_high_pri;

int main() {
    rtos_test::setup();

    mutex0.emplace();
    mutex1.emplace();

    task_high_pri.emplace(2, false, []{
        CHECKPOINT(1);
        rtos::task::suspend();
        CHECKPOINT(5);
        rtos::task::yield();
        CHECKPOINT(7);
        rtos::task::yield();
        CHECKPOINT(9);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task_med_pri(1, false, []{
        CHECKPOINT(2);

        // Ensure the low priority task gets the mutex
        rtos::task::sleep(5);

        FAIL("Task should never run after sleeping");
    });

    rtos_test::TaskWithStack task_low_pri(0, false, []{
        CHECKPOINT(3);

        EXPECT(mutex0->trylock());
        EXPECT(mutex1->trylock());

        CHECKPOINT(4);
    
        rtos::task::resume(&task_high_pri.value());
        rtos::task::yield();

        CHECKPOINT(6);

        // Unlock but stay at high priority due to still holding mutex1
        mutex0->unlock();
        rtos::task::yield();

        CHECKPOINT(8);

        // Unlock and return to low priority
        mutex1->unlock();

        FAIL("Task should be preempted");
    });

    rtos::start();
}
