#include "rtos_test.hh"

inline bool fp_active() {
    return (__get_CONTROL() & (1U << 2U)) != 0;
}

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack fp_task(0, false, []{
        volatile float val = 0.0;
        while (val < 100.0) {
            val += 1.0;
            rtos::task::yield();
            EXPECT(fp_active());
        }
        rtos_test::pass();
    });

    rtos_test::TaskWithStack nonfp_task(0, false, []{
        while (true) {
            rtos::task::yield();
            EXPECT(!fp_active());
        }
    });

    rtos::start();
}
