#pragma once

#include "rtos.h" // IWYU pragma: export
#include "rtos.hh" // IWYU pragma: export
#include "stm32f4xx_hal.h"

#include <array>
#include <cstddef>
#include <source_location>
#include <string_view>
#include <type_traits>

#define EXPECT(cond) \
    do { \
        if (!(cond)) { \
            rtos_test::fail("Expected " #cond); \
        } \
    } while (false)

namespace rtos_test {

template<typename T>
    requires std::is_trivially_destructible_v<T>
class StaticStorage {
public:
    StaticStorage() noexcept = default;

    template<typename... Args>
    auto emplace(Args&&... args) -> T& {
        ::new (storage.data()) T(std::forward<Args>(args)...);
        return *reinterpret_cast<T *>(storage.data());
    }
private:
    alignas(alignof(T)) std::array<std::byte, sizeof(T)> storage{};
};

template<size_t stack_size = 512>
struct TaskWithStack {

    [[nodiscard]] TaskWithStack(size_t priority, bool privileged, void *arg,
                                rtos_task_func_t func):
        task(rtos::Task::Settings{
            .priority = priority,
            .privileged = privileged,
            .stack = stack,
            .arg = arg,
            .function = func,
        })
    {}

    [[nodiscard]] TaskWithStack(size_t priority, bool privileged, void (*func)()):
        task(rtos::Task::Settings{
            .priority = priority,
            .privileged = privileged,
            .stack = stack,
            .arg = nullptr,
            .function = reinterpret_cast<rtos_task_func_t>(func),
        })
    {}

    rtos::Task::Stack<stack_size> stack{};
    rtos::Task task;
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
