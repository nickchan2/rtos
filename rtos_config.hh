#pragma once

#include <array>
#include <cassert>
#include <cstring>
#include <optional>
#include <span>

using TaskFunction = void(*)();


template<std::array keys>
struct FastLookup {
    // std::array<const char *, 2> keys;

    static constexpr bool str_equal(const char *a, const char *b) {
        while (*a && *b) {
            if (*a != *b) return false;
            ++a;
            ++b;
        }
        return *a == *b; // Both should be null-terminated
    }

    constexpr std::optional<int> operator[](int key) const {
        for (size_t i = 0; i < keys.size(); ++i) {
            if (keys[i] == key) {
                return static_cast<int>(i);
            }
        }
        return std::nullopt;
    }
};

constexpr FastLookup<{1, 2}> fast_lookup{};

constexpr int test_val = *fast_lookup[2];



enum class DeadlineMissPolicy {
    finish,
    abort,
};

struct PeriodicTask {
    TaskFunction function;
    size_t period;
    size_t deadline;
    DeadlineMissPolicy miss_policy;

    constexpr PeriodicTask(
        TaskFunction function,
        size_t period,
        size_t deadline,
        DeadlineMissPolicy miss_policy
    ) : function(function),
        period(period),
        deadline(deadline),
        miss_policy(miss_policy) {
        assert(period > 0);
        assert(deadline <= period);
        if (deadline == period) {
            assert(miss_policy == DeadlineMissPolicy::abort);
        }
    }
};

constexpr auto task = PeriodicTask(nullptr, 2, 1, DeadlineMissPolicy::finish);



struct PeriodicTaskBase {
    TaskFunction function;
    size_t period;
    size_t deadline;
    DeadlineMissPolicy miss_policy;
    std::span<std::byte> stack_view;

    // PeriodicTaskBase(
    //     TaskFunction function,
    //     size_t period,
    //     size_t deadline,
    //     DeadlineMissPolicy miss_policy,
    //     std::span<std::byte> stack_view
    // ) : function(function),
    //     period(period),
    //     deadline(deadline),
    //     miss_policy(miss_policy),
    //     stack_view(stack_view) {}
};

template<TaskFunction function_,
         size_t period_,
         size_t deadline_,
         DeadlineMissPolicy miss_policy_,
         size_t stack_size_>
struct PeriodicTaskStorage : public PeriodicTaskBase {
    static_assert(stack_size_ > 0, "Stack size must be greater than 0");
    static_assert(stack_size_ % 8 == 0, "Stack size must be a multiple of 8");
    static_assert(period_ > 0, "Period must be greater than 0");
    static_assert(deadline_ <= period_, "Deadline must be less than or equal to period");
    static_assert(miss_policy_ == DeadlineMissPolicy::abort || deadline_ < period_, "If deadline is equal to period, miss policy must be abort");

    alignas(8) std::array<std::byte, stack_size_> stack;

    PeriodicTaskStorage()
        : PeriodicTaskBase{function_, period_, deadline_, miss_policy_, stack}
    {}
};

void task0_function();
void task1_function();

inline PeriodicTaskStorage<task0_function, 1, 1, DeadlineMissPolicy::abort, 128> task0_storage;
inline PeriodicTaskStorage<task1_function, 1, 1, DeadlineMissPolicy::abort, 128> task1_storage;

constexpr std::array<PeriodicTaskBase *, 2> periodic_tasks = {
    &task0_storage,
    &task1_storage,
};

template<size_t period_task_cnt>
class Rtos {
public:

};
