#include "rtos_test.hh"

static bool fp_active() {
    return (__get_CONTROL() & 0x2) != 0;
}

int main() {
    rtos_test::setup();

    rtos::TaskWithStack fp_task(1, []{
        volatile float val = 0.0;
        while (val < 100.0) {
            val += 1.0;
            rtos::Task::yield();
            EXPECT(fp_active());
        }
    });

    rtos::TaskWithStack nonfp_task(1, []{
        EXPECT(!fp_active());
        while (true) {
            rtos::Task::yield();
            EXPECT(!fp_active());
        }
    });

    rtos::start();
}
