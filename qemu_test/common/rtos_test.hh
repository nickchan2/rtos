#pragma once

#include "rtos.h" // IWYU pragma: export
#include "rtos.hh" // IWYU pragma: export
#include "stm32f4xx_hal.h" // IWYU pragma: export

#include <cstddef>

#define FAIL(msg) rtos_test::fail(msg, __FILE__, __LINE__)

#define EXPECT(cond) \
    do { \
        if (!(cond)) { \
            FAIL("Expected " #cond); \
        } \
    } while (false)

#define CHECKPOINT(num) rtos_test::checkpoint(num, __FILE__, __LINE__)

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

void setup();

void set_timer_callback(void (*callback)());

void start_timer();

[[noreturn]] void pass();

[[noreturn]] void expect_hardfault_to_pass(void (*func)());

[[noreturn]] void fail(const char *msg, const char *file, int line);

void checkpoint(int num, const char *file, int line);

} // namespace rtos_test

extern "C" [[gnu::used]] inline void rtos_test_pass_asm() {
    rtos_test::pass();
}

extern "C" [[gnu::used]] inline void rtos_test_fail_asm() {
    rtos_test::fail("Failed in asm", "<unknown>", 0);
}
