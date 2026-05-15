#pragma once

#include "rtos.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace rtos {

constexpr size_t ticks_per_slice = RTOS_TICKS_PER_SLICE;

[[noreturn]] inline auto start() -> void { rtos_start(); };

inline auto tick() -> void { rtos_tick(); };

struct Task {
    static constexpr size_t min_stack_size = 256;
    static constexpr size_t stack_alignment = 8;

    using TaskFunc = rtos_task_func_t;

    template<size_t size>
        requires (size >= min_stack_size && size % stack_alignment == 0)
    struct Stack {
        alignas(stack_alignment) std::array<std::byte, size> data;
    };

    template<size_t stack_size>
    struct Settings {
        uint32_t priority;
        bool privileged;
        Stack<stack_size> &stack;
        void *arg;
        TaskFunc function;
    };

    template<size_t stack_size>
    Task(const Settings<stack_size> &settings) {
        const auto s = rtos_task_settings_t{
            .function = settings.function,
            .task_arg = settings.arg,
            .stack_low = settings.stack.data.data(),
            .stack_size = settings.stack.data.size(),
            .priority = settings.priority,
            .privileged = settings.privileged,
        };
        rtos_task_create(&tcb, &s);
    }

    static auto yield() -> void { rtos_task_yield(); }

    static auto sleep_for(uint32_t ticks) -> void { rtos_task_sleep(ticks); }

    static auto suspend() -> void { rtos_task_suspend(); }

    static auto resume(Task &task) -> void { rtos_task_resume(&task.tcb); }

    static auto self() -> Task & {
        return *reinterpret_cast<Task *>(rtos_task_self());
    }

    Task() = delete;
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    Task(Task &&) = delete;
    Task &operator=(Task &&) = delete;
private:
    rtos_tcb_t tcb;
};

class Mutex {
public:
    Mutex() { rtos_mutex_create(&mutex, RTOS_MAX_TASK_PRIORITY); }
    Mutex(size_t priority_ceil) { rtos_mutex_create(&mutex, priority_ceil); }
    ~Mutex() { rtos_mutex_destroy(&mutex); }
    
    auto lock() -> void { rtos_mutex_lock(&mutex); }
    auto unlock() -> void { rtos_mutex_unlock(&mutex); }
    auto trylock() -> bool { return rtos_mutex_trylock(&mutex); }
    
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;
    Mutex(Mutex &&) = delete;
    Mutex &operator=(Mutex &&) = delete;
private:
    friend class Cond;
    rtos_mutex_t mutex;
};

class LockGuard {
public:
    explicit LockGuard(Mutex &m) : mutex(m) { mutex.lock(); }
    ~LockGuard() { mutex.unlock(); }

    LockGuard(const LockGuard &) = delete;
    LockGuard &operator=(const LockGuard &) = delete;
    LockGuard(LockGuard &&) = delete;
    LockGuard &operator=(LockGuard &&) = delete;
private:
    Mutex &mutex;
};

class Cond {
public:
    Cond() { rtos_cond_create(&cond); }
    ~Cond() { rtos_cond_destroy(&cond); }

    Cond(const Cond &) = delete;
    Cond &operator=(const Cond &) = delete;
    Cond(Cond &&) = delete;
    Cond &operator=(Cond &&) = delete;

    auto wait(Mutex &mutex) -> void { rtos_cond_wait(&cond, &mutex.mutex); }

    template<typename Predicate>
        requires std::is_invocable_r_v<bool, Predicate>
    auto wait(Mutex &mutex, Predicate pred) -> void {
        while (!pred()) {
            rtos_cond_wait(&cond, &mutex.mutex);
        }
    }

    auto signal() -> void { rtos_cond_signal(&cond); }
    auto broadcast() -> void { rtos_cond_broadcast(&cond); }
private:
    rtos_cond_t cond;
};

template<typename T, size_t slots>
    requires (
        slots > 0 &&
        std::is_trivially_copy_assignable_v<T> &&
        std::is_trivially_move_assignable_v<T>
    )
class Mqueue {
public:
    // Copy semantics when enqueuing, move semantics when dequeuing

    Mqueue() {
        rtos_mqueue_create(
            &mqueue,
            reinterpret_cast<uint8_t *>(storage.data()),
            slots,
            sizeof(T)
        );
    }

    ~Mqueue() { rtos_mqueue_destroy(&mqueue); }

    auto enqueue(const T &data) -> void {
        rtos_mqueue_enqueue(&mqueue, &data);
    }

    auto dequeue() -> T {
        std::array<std::byte, sizeof(T)> raw;
        rtos_mqueue_dequeue(&mqueue, raw.data());
        return *reinterpret_cast<T *>(raw.data());
    }

    auto try_enqueue_isr(const T &data) -> bool {
        return rtos_mqueue_try_enqueue_isr(&mqueue, &data);
    }
private:
    rtos_mqueue_t mqueue;
    std::array<std::byte, slots * sizeof(T)> storage;
};

} // namespace rtos
