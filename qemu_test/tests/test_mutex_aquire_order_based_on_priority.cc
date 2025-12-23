#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

namespace {

std::optional<rtos::Mutex> mutex;

} // namespace

int main() {
    rtos_test::setup();

    mutex.emplace(1);

    rtos_test::TaskWithStack task0(0, false, []{
        rtos_test::checkpoint(3);
        mutex->lock();
        rtos_test::checkpoint(8);
        mutex->unlock();
        rtos_test::checkpoint(9);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        rtos::task::sleep(10);
        rtos_test::checkpoint(4);
        mutex->lock();
        rtos_test::checkpoint(6);
        mutex->unlock();
        rtos_test::checkpoint(7);
    });

    rtos_test::TaskWithStack task2(1, false, []{
        rtos_test::checkpoint(1);
        mutex->lock();
        rtos_test::checkpoint(2);
        rtos::task::sleep(20);
        rtos_test::checkpoint(5);
        mutex->unlock();
    });

    rtos::start();
}
