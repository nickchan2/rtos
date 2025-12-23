#include "rtos.hh"
#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack task0(0, false, []{
        rtos::Mutex mutex;
        mutex.lock();
        mutex.unlock();
        rtos_test::pass();
    });

    rtos::start();
}
