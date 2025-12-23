#include "rtos_test.hh"

#include <optional>

constexpr int waiter_cnt = 4;

namespace {

static volatile int counter = 0;
static std::optional<rtos::Cond> cond;
static std::optional<rtos::Mutex> mutex;

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

    rtos_test::TaskWithStack signaler(0, false, []{
        // Ensure all waiters are waiting before signaling
        rtos::task::sleep(100);

        int last_count = 0;
        for (int i = 0; i < waiter_cnt; ++i) {
            mutex->lock();
            cond->signal();
            mutex->unlock();
            rtos::task::yield();
            mutex->lock();
            EXPECT(counter == ++last_count);
            mutex->unlock();
        }
        rtos_test::pass();
    });

    rtos::start();
}
