#include "rtos_test.hh"

#include <stddef.h>

static volatile bool task0_entered = false;
static volatile bool task1_entered = false;

int main() {
    rtos_test::setup();
    rtos::TaskWithStack task0(2, []{ task0_entered = true; });
    rtos::TaskWithStack task1(2, []{ task1_entered = true; });
    rtos::TaskWithStack pass_task(1, [] {
        EXPECT(task0_entered);
        EXPECT(task1_entered);
        rtos_test::pass();
    });
    rtos::start();
}
