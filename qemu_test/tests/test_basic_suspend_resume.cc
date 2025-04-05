#include "rtos_test.hh"

static void resumer_function(void *suspended_task) {
    // Ensure the suspended task is suspended before continuing
    rtos_task_yield();

    rtos_task_sleep(100);
    rtos_task_resume(static_cast<rtos_task *>(suspended_task));
}

static void suspended_function() {
    const int time_before_suspend = HAL_GetTick();
    rtos_task_suspend();
    int const elapsed = HAL_GetTick() - time_before_suspend;
    assert(elapsed >= 100);
    test_passed();
}

int main() {
    quick_setup();

    Task suspended_task(1, suspended_function);
    Task resumer_task(1, &suspended_task.task, resumer_function);

    rtos_start();
}
