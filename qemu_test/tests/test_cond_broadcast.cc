#include "rtos_test.hh"

#include <optional>

constexpr int waiter_cnt = 4;

namespace {

volatile int counter = 0;
std::optional<rtos::Cond> cond;
std::optional<rtos::Mutex> mutex;

} // namespace

int main() {
    rtos_test::setup();
    cond.emplace();
    mutex.emplace();
    
    std::array<std::optional<rtos_test::TaskWithStack<>>, waiter_cnt> waiters;
    for (auto &waiter : waiters) {
        waiter.emplace(0, false, []{
            mutex->lock();
            cond->wait(*mutex);
            counter = counter + 1;
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
