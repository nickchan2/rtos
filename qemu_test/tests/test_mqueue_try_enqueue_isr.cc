#include "rtos_test.hh"

#include <optional>

namespace {

std::optional<rtos::Mqueue<int, 1>> mqueue;

} // namespace

int main() {
    rtos_test::setup();

    mqueue.emplace();

    rtos_test::TaskWithStack task0(0, false, []{
        rtos_test::checkpoint(2);
        rtos_test::start_timer();
        EXPECT(mqueue->dequeue() == 5678);
        EXPECT(mqueue->dequeue() == 8765);
        rtos_test::checkpoint(6);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        rtos_test::checkpoint(1);
        EXPECT(mqueue->dequeue() == 1234);
        rtos_test::checkpoint(4);
        rtos_test::start_timer();
    });

    rtos_test::set_timer_callback([]{
        static int count = 0;
        EXPECT(mqueue.has_value());
        if (count == 0) {
            rtos_test::checkpoint(3);
            EXPECT(mqueue->try_enqueue_isr(1234));
        } else if (count == 1) {
            rtos_test::checkpoint(5);
            EXPECT(mqueue->try_enqueue_isr(5678));
            EXPECT(mqueue->try_enqueue_isr(8765));
            EXPECT(!mqueue->try_enqueue_isr(4321));
        } else {
            rtos_test::fail("Should not be reached");
        }
        ++count;
    });

    rtos::start();
}
