#include "rtos_test.hh"

#include <stddef.h>

static volatile bool task0_entered = false;
static volatile bool task1_entered = false;

int main() {
    rtos_test::setup();
    rtos_test::TaskWithStack task0(2, false, []{ task0_entered = true; });
    rtos_test::TaskWithStack task1(2, false, []{ task1_entered = true; });
    rtos_test::TaskWithStack pass_task(1, false, [] {
        EXPECT(task0_entered);
        EXPECT(task1_entered);
        rtos_test::pass();
    });
    rtos::start();
}
