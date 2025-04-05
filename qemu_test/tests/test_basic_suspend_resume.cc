#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

static std::optional<rtos_test::TaskWithStack<>> suspended0;
static std::optional<rtos_test::TaskWithStack<>> suspended1;

int main() {
    rtos_test::setup();

    suspended0.emplace(0, false, []{
        CHECKPOINT(2);
        rtos::task::suspend();
        CHECKPOINT(7);
        rtos_test::pass();
    });

    suspended1.emplace(1, false, []{
        CHECKPOINT(1);
        rtos::task::suspend();
        CHECKPOINT(6);
    });

    rtos_test::TaskWithStack resumer(0, false, []{
        // Ensure the suspended task is suspended before continuing
        rtos::task::yield();
        CHECKPOINT(3);
        rtos::task::sleep(100);
        CHECKPOINT(4);
        rtos::task::resume(&suspended0.value());
        CHECKPOINT(5);
        rtos::task::resume(&suspended1.value());
    });

    rtos::start();
}
