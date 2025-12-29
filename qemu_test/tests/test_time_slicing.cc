#include "rtos.hh"
#include "rtos_test.hh"

namespace {

volatile int counter = 0;

auto task_function() -> void {
    while (true) {
        counter = counter + 1;
        const int last_count = counter;
        const uint32_t elapsed = rtos_test::measure_ticks([&]{
            while (counter == last_count) {}
        });
        EXPECT(elapsed >= (static_cast<int>(rtos::ticks_per_slice) - 1) * 2);

        if (counter >= 10) {
            rtos_test::pass();
        }
    }
}

} // namespace

auto main() -> int {
    rtos_test::setup();
    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(1, false, task_function);
    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(1, false, task_function);
    rtos::start();
}
