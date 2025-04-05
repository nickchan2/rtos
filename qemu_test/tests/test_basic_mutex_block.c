#include "rtos.h"
#include "test_utils.h"

static struct rtos_mutex mutex;
static volatile int counter = 0;

static void task_function(void *)
{
    rtos_mutex_lock(&mutex);
    ++counter;
    rtos_task_sleep(10);
    const bool is_done = counter == 2;
    rtos_mutex_unlock(&mutex);
    if (is_done) {
        test_passed();
    }
}

int main(void)
{
    quick_setup();
    rtos_mutex_create(&mutex);
    struct rtos_task task[2];
    stack_512_t stacks[2];
    for (int i = 0; i < 2; ++i) {
        rtos_task_create(&task[i], &(struct rtos_task_settings){
            .function = task_function,
            .task_arg = NULL,
            .stack_low = &stacks[i],
            .stack_size = sizeof(stacks[i]),
            .priority = 1,
        });
    }
    rtos_start();
}
