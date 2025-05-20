#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();
    mutex.emplace(1);

    rtos_test::TaskWithStack task0(1, false, []{
        CHECKPOINT(1);
        rtos::task::suspend();
        CHECKPOINT(4);
        mutex->lock();
        CHECKPOINT(6);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        rtos::task::sleep(10);
        while (true) {}
    });

    rtos_test::TaskWithStack task2(0, false, &task0, [](void *arg){
        CHECKPOINT(2);
        mutex->lock();
        CHECKPOINT(3);
        rtos::task::resume(static_cast<rtos::Task *>(arg));
        rtos::task::sleep(20);
        CHECKPOINT(5);
        mutex->unlock();
        FAIL("Task should be preempted");
    });

    rtos::start();
}
