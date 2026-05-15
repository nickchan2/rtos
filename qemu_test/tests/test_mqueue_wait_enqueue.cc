#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

constexpr int enqueue_task_count = 10;

auto main() -> int {
    rtos_test::setup();

    static auto mutex = rtos::Mutex();
    static auto mqueue = rtos::Mqueue<int, 1>();
    static volatile int counter = 0;

    static auto tasks = std::array<std::optional<rtos_test::TaskWithStack<>>, enqueue_task_count>();
    for (auto &task : tasks) {
        task.emplace(0, false, []{
            int local_counter = 0;
            mutex.lock();
            local_counter = counter;
            counter = counter + 1;
            mutex.unlock();
            mqueue.enqueue(local_counter);
            rtos::Task::suspend();
        });
    }

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos::Task::sleep_for(10);
        mutex.lock();
        EXPECT(counter == enqueue_task_count);
        mutex.unlock();
        for (int i = 0; i < enqueue_task_count; ++i) {
            EXPECT(mqueue.dequeue() == i);
        }
        rtos_test::pass();
    });

    rtos::start();
}
