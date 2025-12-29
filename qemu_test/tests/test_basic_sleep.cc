#include "rtos_test.hh"

#include <cstdint>

auto main() -> int {
    rtos_test::setup();
    [[maybe_unused]] static auto task = rtos_test::TaskWithStack(0, false, []{
        for (int i = 0; i < 10; ++i) {
            const uint32_t elapsed = rtos_test::measure_ticks([] {
                rtos::task::sleep(5);
            });
            EXPECT(elapsed >= 5);
        }
        rtos_test::pass();
    });
    rtos::start();
}
