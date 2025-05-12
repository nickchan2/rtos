#include "rtos_test.hh"

int main() {
    rtos_test::setup();

    rtos_test::TaskWithStack privileged(1, true, []{
        CHECKPOINT(1);
        __disable_irq();
        __enable_irq();
    });

    rtos_test::TaskWithStack unprivileged(0, false, []{
        CHECKPOINT(2);
        rtos_test::expect_hardfault_to_pass([]{ __disable_irq(); });
    });

    rtos::start();
}
