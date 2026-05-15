#include "rtos_test.hh"

inline auto fp_active() -> bool {
    return (__get_CONTROL() & (1U << 2U)) != 0;
}

auto main() -> int {
    rtos_test::setup();

    [[maybe_unused]] static auto fp_task = rtos_test::TaskWithStack(0, false, []{
        volatile float val = 0.0;
        while (val < 100.0) {
            val += 1.0;
            rtos::Task::yield();
            EXPECT(fp_active());
        }
        rtos_test::pass();
    });

    [[maybe_unused]] static auto nonfp_task = rtos_test::TaskWithStack(0, false, []{
        while (true) {
            rtos::Task::yield();
            EXPECT(!fp_active());
        }
    });

    rtos::start();
}
