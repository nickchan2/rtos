#pragma once

#include "rtos.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace rtos {

constexpr size_t ticks_per_slice = RTOS_TICKS_PER_SLICE;

[[noreturn]] inline void start() { rtos_start(); };

inline void tick() { rtos_tick(); };

struct Task : public rtos_tcb_t {
    using Settings = rtos_task_settings_t;

    Task() = default;

    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;
    Task(Task &&) = delete;
    Task &operator=(Task &&) = delete;
};

namespace task {

    inline void create(Task &task, const Task::Settings &settings) {
        rtos_task_create(&task, &settings);
    }
    inline void yield() { rtos_task_yield(); }
    inline void sleep(size_t ticks) { rtos_task_sleep(ticks); }
    inline void suspend() { rtos_task_suspend(); }
    inline void resume(Task *task) { rtos_task_resume(task); }
    inline Task *self() { return reinterpret_cast<Task *>(rtos_task_self()); }
    [[noreturn]] inline void exit() { rtos_task_exit(); }
    inline void join(Task *task) { rtos_task_join(task); }

} // namespace task

struct Mutex {
    rtos_mutex_t mutex;

    Mutex() { rtos_mutex_create(&mutex, RTOS_MAX_TASK_PRIORITY); }
    Mutex(size_t priority_ceil) { rtos_mutex_create(&mutex, priority_ceil); }
    ~Mutex() { rtos_mutex_destroy(&mutex); }
    void lock() { rtos_mutex_lock(&mutex); }
    void unlock() { rtos_mutex_unlock(&mutex); }
    bool trylock() { return rtos_mutex_trylock(&mutex); }
};

struct Cond {
    rtos_cond_t cond;

    Cond() { rtos_cond_create(&cond); }
    ~Cond() { rtos_cond_destroy(&cond); }
    void wait(Mutex &mutex) { rtos_cond_wait(&cond, &mutex.mutex); }
    void signal() { rtos_cond_signal(&cond); }
    void broadcast() { rtos_cond_broadcast(&cond); }
};

template<typename T, size_t slots>
struct Mqueue {
    static_assert(std::is_trivially_copy_assignable_v<T>);
    static_assert(std::is_trivially_move_assignable_v<T>);

    rtos_mqueue_t mqueue;
    std::array<uint8_t, slots * sizeof(T)> storage;

    Mqueue() { rtos_mqueue_create(&mqueue, storage.data(), slots, sizeof(T)); }
    ~Mqueue() { rtos_mqueue_destroy(&mqueue); }

    void enqueue(const T &data) { rtos_mqueue_enqueue(&mqueue, &data); }

    T dequeue() {
        std::byte raw[sizeof(T)];
        rtos_mqueue_dequeue(&mqueue, raw);
        return *reinterpret_cast<T *>(raw);
    }

    bool try_enqueue_isr(const T &data) {
        return rtos_mqueue_try_enqueue_isr(&mqueue, &data);
    }
};

} // namespace rtos
