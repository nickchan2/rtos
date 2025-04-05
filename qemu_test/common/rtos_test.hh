#pragma once

#include "rtos.h" // IWYU pragma: export
#include "stm32f4xx_hal.h" // IWYU pragma: export

#include <array>
#include <cstddef>

struct Task {
    using TaskFuncNoArg = void (*)(void);

    alignas(8) std::array<std::byte, 512> stack;
    rtos_task task;

    [[nodiscard]] Task(size_t priority, void *arg, rtos_task_func_t func) {
        const rtos_task_settings settings = {
            .function = func,
            .task_arg = arg,
            .stack_low = stack.data(),
            .stack_size = stack.size(),
            .priority = priority,
        };
        rtos_task_create(&task, &settings);
    }

    [[nodiscard]] Task(size_t priority, TaskFuncNoArg func) {
        const rtos_task_settings settings = {
            .function = reinterpret_cast<rtos_task_func_t>(func),
            .task_arg = nullptr,
            .stack_low = stack.data(),
            .stack_size = stack.size(),
            .priority = priority,
        };
        rtos_task_create(&task, &settings);
    }
};

void quick_setup(void);

void uart_init(void);

void configure_nvic_for_rtos(void);

void test_passed(void);
