#pragma once

#include "rtos.h"

#include <array>

namespace rtos {

inline void start() { rtos_start(); };

inline void tick() { rtos_tick(); };

struct Task : public rtos_task {
    using Settings = rtos_task_settings;

    static void create(Task &task, const Settings &settings) {
        rtos_task_create(&task, &settings);
    }
    static void yield() { rtos_task_yield(); }
    static void sleep(size_t ticks) { rtos_task_sleep(ticks); }
    static void suspend() { rtos_task_suspend(); }
    static void resume(Task *task) { rtos_task_resume(task); }
    static Task *self() { return reinterpret_cast<Task *>(rtos_task_self()); }
    static void exit() { rtos_task_exit(); }

    Task() = default;

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    Task(Task &&) = delete;
    Task &operator=(Task &&) = delete;
};

template<size_t stack_size = 512>
struct TaskWithStack : public Task {
    static_assert(stack_size % 8 == 0, "Stack size must be a multiple of 8");
    static_assert(stack_size >= 256, "Stack size must be at least 256 bytes");

    [[nodiscard]] TaskWithStack(size_t priority, void *arg, rtos_task_func_t func) {
        create(*this, {
            .function = func,
            .task_arg = arg,
            .stack_low = stack.data(),
            .stack_size = stack.size(),
            .priority = priority,
        });
    }

    [[nodiscard]] TaskWithStack(size_t priority, void (*func)()) {
        create(*this, {
            .function = reinterpret_cast<rtos_task_func_t>(func),
            .task_arg = nullptr,
            .stack_low = stack.data(),
            .stack_size = stack.size(),
            .priority = priority,
        });
    }

    alignas(8) std::array<std::byte, stack_size> stack;
};

struct Mutex {
    rtos_mutex mutex;

    Mutex() { rtos_mutex_create(&mutex); }
    ~Mutex() { rtos_mutex_destroy(&mutex); }
    void lock() { rtos_mutex_lock(&mutex); }
    void unlock() { rtos_mutex_unlock(&mutex); }
    bool trylock() { return rtos_mutex_trylock(&mutex); }
};

struct Cond {
    rtos_cond cond;

    Cond() { rtos_cond_create(&cond); }
    ~Cond() { rtos_cond_destroy(&cond); }
    void wait(Mutex &mutex) { rtos_cond_wait(&cond, &mutex.mutex); }
    void signal() { rtos_cond_signal(&cond); }
    void broadcast() { rtos_cond_broadcast(&cond); }
};

} // namespace rtos
