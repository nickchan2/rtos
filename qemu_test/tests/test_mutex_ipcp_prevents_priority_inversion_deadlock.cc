#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

namespace {

std::optional<rtos::Mutex> mutex;

} // namespace

int main() {
    rtos_test::setup();
    mutex.emplace(1);

    rtos_test::TaskWithStack task0(1, false, []{
        rtos_test::checkpoint(1);
        rtos::task::suspend();
        rtos_test::checkpoint(4);
        mutex->lock();
        rtos_test::checkpoint(6);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        rtos::task::sleep(10);
        while (true) {}
    });

    rtos_test::TaskWithStack task2(0, false, &task0, [](void *arg){
        rtos_test::checkpoint(2);
        mutex->lock();
        rtos_test::checkpoint(3);
        rtos::task::resume(static_cast<rtos::Task *>(arg));
        rtos::task::sleep(20);
        rtos_test::checkpoint(5);
        mutex->unlock();
        rtos_test::fail("Task should be preempted");
    });

    rtos::start();
}
