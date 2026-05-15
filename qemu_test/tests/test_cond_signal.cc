#include "rtos_test.hh"

#include <optional>

constexpr int waiter_cnt = 4;

auto main() -> int {
    rtos_test::setup();

    static auto cond = rtos::Cond();
    static auto mutex = rtos::Mutex();
    static volatile int counter = 0;

    static auto waiters = std::array<std::optional<rtos_test::TaskWithStack<>>, waiter_cnt>();
    for (auto &waiter : waiters) {
        waiter.emplace(0, false, []{
            mutex.lock();
            cond.wait(mutex);
            counter = counter + 1;
            mutex.unlock();
            rtos::Task::suspend();
        });
    }

    rtos_test::TaskWithStack signaler(0, false, []{
        // Ensure all waiters are waiting before signaling
        rtos::Task::sleep_for(100);

        int last_count = 0;
        for (int i = 0; i < waiter_cnt; ++i) {
            mutex.lock();
            cond.signal();
            mutex.unlock();
            rtos::Task::yield();
            mutex.lock();
            EXPECT(counter == ++last_count);
            mutex.unlock();
        }
        rtos_test::pass();
    });

    rtos::start();
}
