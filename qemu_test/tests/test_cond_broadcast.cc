#include "rtos_test.hh"

#include <optional>

static const int waiter_cnt = 4;

static volatile int counter = 0;
static std::optional<rtos::Cond> cond;
static std::optional<rtos::Mutex> mutex;

int main() {
    rtos_test::setup();
    cond.emplace();
    mutex.emplace();
    
    std::array<std::optional<rtos_test::TaskWithStack<>>, waiter_cnt> waiters;
    for (auto &waiter : waiters) {
        waiter.emplace(0, false, []{
            mutex->lock();
            cond->wait(*mutex);
            ++counter;
            mutex->unlock();
        });
    }

    rtos_test::TaskWithStack broadcaster(0, false, []{
        // Ensure all waiters are waiting before signaling
        rtos::task::sleep(100);

        mutex->lock();
        cond->broadcast();
        mutex->unlock();
        rtos::task::sleep(100);
        mutex->lock();
        EXPECT(counter == waiter_cnt);
        mutex->unlock();
        rtos_test::pass();
    });

    rtos::start();
}
