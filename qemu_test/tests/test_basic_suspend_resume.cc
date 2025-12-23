#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

namespace {

std::optional<rtos_test::TaskWithStack<>> suspended0;
std::optional<rtos_test::TaskWithStack<>> suspended1;

} // namespace

int main() {
    rtos_test::setup();

    suspended0.emplace(0, false, []{
        rtos_test::checkpoint(2);
        rtos::task::suspend();
        rtos_test::checkpoint(7);
        rtos_test::pass();
    });

    suspended1.emplace(1, false, []{
        rtos_test::checkpoint(1);
        rtos::task::suspend();
        rtos_test::checkpoint(6);
    });

    rtos_test::TaskWithStack resumer(0, false, []{
        // Ensure the suspended task is suspended before continuing
        rtos::task::yield();
        rtos_test::checkpoint(3);
        rtos::task::sleep(100);
        rtos_test::checkpoint(4);
        rtos::task::resume(&suspended0.value());
        rtos_test::checkpoint(5);
        rtos::task::resume(&suspended1.value());
    });

    rtos::start();
}
