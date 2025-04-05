#include "rtos.hh"
#include "rtos_test.hh"

#include <cstdint>
#include <optional>

std::optional<rtos::Mqueue<int32_t, 1>> mqueue_i32;
std::optional<rtos::Mqueue<int64_t, 1>> mqueue_i64;

int main() {
    rtos_test::setup();

    mqueue_i32.emplace();
    mqueue_i64.emplace();
    
    rtos_test::TaskWithStack task0(0, false, []{
        int counter = 0;
        for (int i = 0; i < 10; ++i) {
            ++counter;
            mqueue_i32->enqueue(counter);
            const int64_t counter_incremented = mqueue_i64->dequeue();
            EXPECT(counter_incremented == ++counter);
        }
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(0, false, []{
        while (true) {
            int32_t counter = mqueue_i32->dequeue();
            ++counter;
            mqueue_i64->enqueue(counter);
        }
    });

    rtos::start();
}
