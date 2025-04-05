#include "rtos.hh"
#include "rtos_test.hh"

static volatile int counter = 0;

static void task_function() {
    while (true) {
        ++counter;
        const int last_count = counter;
        const int last_time = HAL_GetTick();
        while (counter == last_count) {}
        const int elapsed = HAL_GetTick() - last_time;
        EXPECT(elapsed >= (static_cast<int>(rtos::ticks_per_slice) - 1) * 2);

        if (counter >= 10) {
            rtos_test::pass();
        }
    }
}

int main() {
    rtos_test::setup();
    rtos_test::TaskWithStack task0(1, false, task_function);
    rtos_test::TaskWithStack task1(1, false, task_function);
    rtos::start();
}
