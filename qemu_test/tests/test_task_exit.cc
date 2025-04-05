#include "rtos_test.hh"

#include <stddef.h>

static volatile bool task0_entered = false;
static volatile bool task1_entered = false;

int main() {
    quick_setup();
    Task task0(2, []{ task0_entered = true; });
    Task task1(2, []{ task1_entered = true; });
    Task pass_task(1, [] {
        EXPECT(task0_entered);
        EXPECT(task1_entered);
        test_passed();
    });
    rtos_start();
}
