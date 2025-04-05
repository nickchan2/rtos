#include "rtos_test.hh"

static struct rtos_mutex mutex;
static volatile int counter = 0;

static void task_function() {
    rtos_mutex_lock(&mutex);
    ++counter;
    rtos_task_sleep(10);
    const bool is_done = counter == 2;
    rtos_mutex_unlock(&mutex);
    if (is_done) {
        test_passed();
    }
}

int main() {
    quick_setup();
    rtos_mutex_create(&mutex);
    Task task0(1, task_function);
    Task task1(1, task_function);
    rtos_start();
}
