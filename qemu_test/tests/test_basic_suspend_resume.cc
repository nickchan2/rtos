#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos::TaskWithStack suspended(1, []{
        CHECKPOINT(1);
        rtos::task::suspend();
        CHECKPOINT(4);
        rtos_test::pass();
    });

    rtos::TaskWithStack resumer(1, &suspended, [](void *suspended){
        // Ensure the suspended task is suspended before continuing
        rtos::task::yield();
        CHECKPOINT(2);
        rtos::task::sleep(100);
        CHECKPOINT(3);
        rtos::task::resume(static_cast<rtos::Task *>(suspended));
    });

    rtos::start();
}
