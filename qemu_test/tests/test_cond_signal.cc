#include "rtos_test.hh"

#include <optional>

constexpr int waiter_count = 4;

static volatile int counter = 0;
static std::optional<Cond> cond;
static std::optional<Mutex> mutex;

int main() {
    quick_setup();
    cond.emplace();
    mutex.emplace();

    std::array<std::optional<Task>, waiter_count> waiter_tasks;
    for (auto &task : waiter_tasks) {
        task.emplace(1, []{
            mutex->lock();
            cond->wait(*mutex);
            ++counter;
            mutex->unlock();
        });
    }

    Task signaler_task(1, []{
        // Ensure all waiters are waiting before signaling
        rtos_task_sleep(100);

        int last_count = 0;
        for (int i = 0; i < waiter_count; ++i) {
            cond->signal();
            rtos_task_yield();
            mutex->lock();
            EXPECT(counter == ++last_count);
            mutex->unlock();
        }
        test_passed();
    });

    rtos_start();
}
