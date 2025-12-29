#include "rtos_test.hh"

namespace {

volatile int counter = 0;

auto task_function() -> void {
    int last = counter;
    while (true) {
        counter = counter + 1;
        if (counter > 100) {
            rtos_test::pass();
        }
        last = counter;
        rtos::task::yield();
        EXPECT(counter == last + 1);
    }
}

} // namespace

auto main() -> int {
    rtos_test::setup();

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(1, false, task_function);
    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(1, false, task_function);

    rtos::start();
}
