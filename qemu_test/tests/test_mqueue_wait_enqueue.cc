#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

constexpr int enqueue_task_count = 10;

namespace {

volatile int counter = 0;
std::optional<rtos::Mutex> mutex;
std::optional<rtos::Mqueue<int, 1>> mqueue;

} // namespace

int main() {
    rtos_test::setup();

    mutex.emplace();
    mqueue.emplace();

    std::array<std::optional<rtos_test::TaskWithStack<>>, enqueue_task_count> tasks;
    for (auto &task : tasks) {
        task.emplace(0, false, []{
            int local_counter = 0;
            mutex->lock();
            local_counter = counter;
            counter = counter + 1;
            mutex->unlock();
            mqueue->enqueue(local_counter);
        });
    }

    rtos_test::TaskWithStack task0(0, false, []{
        rtos::task::sleep(10);
        mutex->lock();
        EXPECT(counter == enqueue_task_count);
        mutex->unlock();
        for (int i = 0; i < enqueue_task_count; ++i) {
            EXPECT(mqueue->dequeue() == i);
        }
        rtos_test::pass();
    });

    rtos::start();
}
