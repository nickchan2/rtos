#include "rtos_test.hh"

static struct rtos_mutex mutex;
static volatile bool task1_tried_to_lock = false;

int main() {
    quick_setup();

    rtos_mutex_create(&mutex);

    Task task0(1, []{
        rtos_mutex_lock(&mutex);
        rtos_task_sleep(10);
        rtos_mutex_unlock(&mutex);
        assert(task1_tried_to_lock);
        test_passed();
    });

    Task task1(1, []{
        const bool got_lock = rtos_mutex_trylock(&mutex);
        assert(got_lock == false);
        task1_tried_to_lock = true;
    });

    rtos_start();
}
