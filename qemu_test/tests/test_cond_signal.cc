#include "rtos_test.hh"

#include <optional>

constexpr int waiter_count = 4;

static volatile int counter = 0;
static std::optional<rtos::Cond> cond;
static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();
    cond.emplace();
    mutex.emplace();

    std::array<std::optional<rtos::TaskWithStack<>>, waiter_count> waiters;
    for (auto &waiter : waiters) {
        waiter.emplace(1, []{
            mutex->lock();
            cond->wait(*mutex);
            ++counter;
            mutex->unlock();
        });
    }

    rtos::TaskWithStack signaler(1, []{
        // Ensure all waiters are waiting before signaling
        rtos::task::sleep(100);

        int last_count = 0;
        for (int i = 0; i < waiter_count; ++i) {
            cond->signal();
            rtos::task::yield();
            mutex->lock();
            EXPECT(counter == ++last_count);
            mutex->unlock();
        }
        rtos_test::pass();
    });

    rtos::start();
}
