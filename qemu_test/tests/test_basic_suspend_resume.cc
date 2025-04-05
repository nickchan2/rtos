#include "rtos_test.hh"

int main() {
    quick_setup();

    Task suspended_task(1, []{
        CHECKPOINT(1);
        rtos_task_suspend();
        CHECKPOINT(4);
        test_passed();
    });

    Task resumer_task(1, &suspended_task.task, [](void *suspended_task){
        // Ensure the suspended task is suspended before continuing
        rtos_task_yield();
        CHECKPOINT(2);
        rtos_task_sleep(100);
        CHECKPOINT(3);
        rtos_task_resume(static_cast<rtos_task *>(suspended_task));
    });

    rtos_start();
}
