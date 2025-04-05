#include "rtos.h"
#include "test_utils.h"

static struct rtos_mutex mutex;
static volatile bool task1_tried_to_lock = false;

static void task0(void *)
{
    rtos_mutex_lock(&mutex);
    rtos_task_sleep(10);
    rtos_mutex_unlock(&mutex);
    assert(task1_tried_to_lock);
    test_passed();
}

static void task1(void *)
{
    const bool got_lock = rtos_mutex_trylock(&mutex);
    assert(got_lock == false);
    task1_tried_to_lock = true;
}

int main(void)
{
    quick_setup();
    rtos_mutex_create(&mutex);
    struct rtos_task tasks[2];
    stack_512_t stacks[2];
    for (int i = 0; i < 2; ++i) {
        rtos_task_create(&tasks[i], &(struct rtos_task_settings){
            .function = (i == 0) ? task0 : task1,
            .task_arg = NULL,
            .stack_low = &stacks[i],
            .stack_size = sizeof(stacks[i]),
            .priority = 1,
        });
    }
    rtos_start();
}
