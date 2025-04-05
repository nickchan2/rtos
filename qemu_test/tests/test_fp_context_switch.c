#include "rtos.h"
#include "test_utils.h"

#include <stdbool.h>

static bool fp_active(void)
{
    return (__get_CONTROL() & 0x2) != 0;
}

static void fp_function(void *)
{
    volatile float val = 0.0;
    while (val < 100.0) {
        val += 1.0;
        rtos_task_yield();
        assert(fp_active());
    }
    test_passed();
}

static void nonfp_function(void *)
{
    assert(!fp_active());
    while (true) {
        rtos_task_yield();
        assert(!fp_active());
    }
}

int main(void)
{
    quick_setup();

    struct rtos_task fp_task;
    stack_512_t fp_stack;
    rtos_task_create(&fp_task, &(struct rtos_task_settings){
        .function = fp_function,
        .task_arg = NULL,
        .stack_low = &fp_stack,
        .stack_size = sizeof(fp_stack),
        .priority = 1,
    });

    struct rtos_task nonfp_task;
    stack_512_t nonfp_stack;
    rtos_task_create(&nonfp_task, &(struct rtos_task_settings){
        .function = nonfp_function,
        .task_arg = NULL,
        .stack_low = &nonfp_stack,
        .stack_size = sizeof(nonfp_stack),
        .priority = 1,
    });

    assert(!fp_active());
    rtos_start();
}
