#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();

    mutex.emplace(1);

    rtos_test::TaskWithStack task0(0, false, []{
        CHECKPOINT(3);
        mutex->lock();
        CHECKPOINT(8);
        mutex->unlock();
        CHECKPOINT(9);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        rtos::task::sleep(10);
        CHECKPOINT(4);
        mutex->lock();
        CHECKPOINT(6);
        mutex->unlock();
        CHECKPOINT(7);
    });

    rtos_test::TaskWithStack task2(1, false, []{
        CHECKPOINT(1);
        mutex->lock();
        CHECKPOINT(2);
        rtos::task::sleep(20);
        CHECKPOINT(5);
        mutex->unlock();
    });

    rtos::start();
}
