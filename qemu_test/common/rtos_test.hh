#pragma once

#include "rtos.h" // IWYU pragma: export
#include "stm32f4xx_hal.h" // IWYU pragma: export

#include <array>
#include <cstddef>

#define FAIL(msg) test_failed(msg, __FILE__, __LINE__)

#define EXPECT(cond) \
    do { \
        if (!(cond)) { \
            FAIL("Expected " #cond); \
        } \
    } while (false)

#define CHECKPOINT(num) checkpoint(num, __FILE__, __LINE__)

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

void quick_setup();

void uart_init();

void configure_nvic_for_rtos();

void test_passed();

void test_failed(const char *msg, const char *file, int line);

void checkpoint(int num, const char *file, int line);
