#include "rtos.hh"
#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();
    [[maybe_unused]] static auto task = rtos_test::TaskWithStack(0, false, []{
        rtos_test::pass();
    });
    rtos::start();
}
