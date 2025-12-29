#pragma once

#include "rtos.h" // IWYU pragma: export
#include "rtos.hh" // IWYU pragma: export
#include "stm32f4xx_hal.h"

#include <array>
#include <cstddef>
#include <source_location>
#include <string_view>

#define EXPECT(cond) \
    do { \
        if (!(cond)) { \
            rtos_test::fail("Expected " #cond); \
        } \
    } while (false)

namespace rtos_test {

template<size_t stack_size = 512>
struct TaskWithStack : public rtos::Task {
    static_assert(stack_size % 8 == 0, "Stack size must be a multiple of 8");
    static_assert(stack_size >= 256, "Stack size must be at least 256 bytes");

    [[nodiscard]] TaskWithStack(size_t priority, bool privileged, void *arg,
                                rtos_task_func_t func)
    {
        rtos::task::create(*this, {
            .function = func,
            .task_arg = arg,
            .stack_low = stack.data(),
            .stack_size = stack.size(),
            .priority = priority,
            .privileged = false,
        });
    }

    [[nodiscard]] TaskWithStack(size_t priority, bool privileged, void (*func)()) {
        rtos::task::create(*this, {
            .function = reinterpret_cast<rtos_task_func_t>(func),
            .task_arg = nullptr,
            .stack_low = stack.data(),
            .stack_size = stack.size(),
            .priority = priority,
            .privileged = false,
        });
    }

    alignas(8) std::array<std::byte, stack_size> stack;
};

auto setup() -> void;

auto set_timer_callback(void (*callback)()) -> void;

auto start_timer() -> void;

[[noreturn]] auto pass() -> void;

[[noreturn]] auto expect_hardfault_to_pass(void (*func)()) -> void;

[[noreturn]] auto fail(
    std::string_view msg,
    std::source_location location = std::source_location::current()
) -> void;

auto checkpoint(
    int num,
    std::source_location location = std::source_location::current()
) -> void;

template<typename Func>
auto measure_ticks(Func&& func) -> uint32_t {
    const uint32_t start = HAL_GetTick();
    func();
    const uint32_t end = HAL_GetTick();
    return end - start;
}

} // namespace rtos_test

extern "C" {

[[gnu::used]] inline auto rtos_test_pass_asm() -> void {
    rtos_test::pass();
}

[[gnu::used]] inline auto rtos_test_fail_asm() -> void {
    rtos_test::fail("Failed in asm");
}

} // extern "C"
