#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos::TaskWithStack suspended(1, []{
        CHECKPOINT(1);
        rtos::Task::suspend();
        CHECKPOINT(4);
        rtos_test::pass();
    });

    rtos::TaskWithStack resumer(1, &suspended, [](void *suspended){
        // Ensure the suspended task is suspended before continuing
        rtos::Task::yield();
        CHECKPOINT(2);
        rtos::Task::sleep(100);
        CHECKPOINT(3);
        rtos::Task::resume(static_cast<rtos::Task *>(suspended));
    });

    rtos::start();
}
