#include "rtos_test.hh"

static volatile int counter = 0;

static void task_function() {
    while (true) {
        ++counter;
        const int last_count = counter;
        const int last_time = HAL_GetTick();
        while (counter == last_count) {}
        const int elapsed = HAL_GetTick() - last_time;
        assert(elapsed >= (RTOS_TICKS_PER_SLICE - 1) * 2);

        if (counter >= 10) {
            test_passed();
        }
    }
}

int main() {
    quick_setup();
    Task task0(1, task_function);
    Task task1(1, task_function);
    rtos_start();
}
