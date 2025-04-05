#include "rtos_test.hh"

#include <optional>

static const int waiter_count = 4;

static volatile int counter = 0;
static struct rtos_cond cond;
static struct rtos_mutex mutex;

int main() {
    quick_setup();
    rtos_cond_create(&cond);
    rtos_mutex_create(&mutex);
    
    std::array<std::optional<Task>, waiter_count> waiter_tasks;
    for (auto &task : waiter_tasks) {
        task.emplace(1, []{
            rtos_mutex_lock(&mutex);
            rtos_cond_wait(&cond, &mutex);
            ++counter;
            rtos_mutex_unlock(&mutex);
        });
    }

    Task signaler_task(1, []{
        // Ensure all waiters are waiting before signaling
        rtos_task_sleep(100);

        rtos_cond_broadcast(&cond);
        rtos_task_sleep(100);
        rtos_mutex_lock(&mutex);
        assert(counter == waiter_count);
        rtos_mutex_unlock(&mutex);
        test_passed();
    });

    rtos_start();
}
