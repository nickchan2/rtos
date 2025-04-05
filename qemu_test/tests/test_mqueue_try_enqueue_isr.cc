#include "rtos_test.hh"

#include <optional>

static std::optional<rtos::Mqueue<int, 1>> mqueue;

int main() {
    rtos_test::setup();

    mqueue.emplace();

    rtos_test::TaskWithStack task0(0, false, []{
        CHECKPOINT(2);
        rtos_test::start_timer();
        EXPECT(mqueue->dequeue() == 5678);
        EXPECT(mqueue->dequeue() == 8765);
        CHECKPOINT(6);
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        CHECKPOINT(1);
        EXPECT(mqueue->dequeue() == 1234);
        CHECKPOINT(4);
        rtos_test::start_timer();
    });

    rtos_test::set_timer_callback([]{
        static int count = 0;
        EXPECT(mqueue.has_value());
        if (count == 0) {
            CHECKPOINT(3);
            EXPECT(mqueue->try_enqueue_isr(1234));
        } else if (count == 1) {
            CHECKPOINT(5);
            EXPECT(mqueue->try_enqueue_isr(5678));
            EXPECT(mqueue->try_enqueue_isr(8765));
            EXPECT(!mqueue->try_enqueue_isr(4321));
        } else {
            FAIL("Should not be reached");
        }
        ++count;
    });

    rtos::start();
}
