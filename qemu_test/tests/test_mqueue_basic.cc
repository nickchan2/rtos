#include "rtos_test.hh"

#include <optional>

struct CustomType {
    int a{2003};
    char b{'b'};
    bool operator==(const CustomType &other) const {
        return a == other.a && b == other.b;
    }
};

static std::optional<rtos::Mqueue<int, 2>> mqueue_int;
static std::optional<rtos::Mqueue<CustomType, 1>> mqueue_custom_type;

int main() {
    rtos_test::setup();

    mqueue_int.emplace();
    mqueue_custom_type.emplace();

    rtos_test::TaskWithStack task0(0, false, []{
        CHECKPOINT(3);
        EXPECT(mqueue_int->dequeue() == 42);
        EXPECT(mqueue_int->dequeue() == -42);
        EXPECT(mqueue_custom_type->dequeue() == CustomType());
        rtos_test::pass();
    });

    rtos_test::TaskWithStack task1(1, false, []{
        CHECKPOINT(1);
        mqueue_int->enqueue(42);
        mqueue_int->enqueue(-42);
        mqueue_custom_type->enqueue(CustomType());
        CHECKPOINT(2);
    });

    rtos::start();
}
