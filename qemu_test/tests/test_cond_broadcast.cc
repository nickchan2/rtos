#include "rtos_test.hh"

#include <optional>

static const int waiter_count = 4;

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

    Task broadcaster_task(1, []{
        // Ensure all waiters are waiting before signaling
        rtos_task_sleep(100);

        cond->broadcast();
        rtos_task_sleep(100);
        mutex->lock();
        EXPECT(counter == waiter_count);
        mutex->unlock();
        test_passed();
    });

    rtos_start();
}
