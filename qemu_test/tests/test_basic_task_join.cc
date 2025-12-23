#include "rtos.hh"
#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack task0(0, false, []{
        rtos_test::checkpoint(3);
        rtos::task::sleep(rtos::ticks_per_slice * 2);
        rtos_test::checkpoint(4);
    });

    rtos_test::TaskWithStack task1(1, false, &task0, [](void *arg){
        rtos_test::checkpoint(2);
        rtos::task::join(static_cast<rtos::Task *>(arg));
        rtos_test::checkpoint(6);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task2(2, false, &task0, [](void *arg){
        rtos_test::checkpoint(1);
        rtos::task::join(static_cast<rtos::Task *>(arg));
        rtos_test::checkpoint(5);
        rtos_test::pass();
    });

    rtos::start();
}
