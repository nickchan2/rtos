#include "rtos_test.hh"

static bool fp_active() {
    return (__get_CONTROL() & 0x2) != 0;
}

int main() {
    quick_setup();

    Task fp_task(1, []{
        volatile float val = 0.0;
        while (val < 100.0) {
            val += 1.0;
            rtos_task_yield();
            assert(fp_active());
        }
    });

    Task nonfp_task(1, []{
        assert(!fp_active());
        while (true) {
            rtos_task_yield();
            assert(!fp_active());
        }
    });

    rtos_start();
}
