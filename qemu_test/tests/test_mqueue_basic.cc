#include "rtos.hh"
#include "rtos_test.hh"

struct CustomType {
    int a{2003};
    char b{'b'};
    bool operator==(const CustomType &other) const {
        return a == other.a && b == other.b;
    }
};

int main() {
    rtos_test::setup();

    static auto mqueue_int = rtos::Mqueue<int, 2>();
    static auto mqueue_custom_type = rtos::Mqueue<CustomType, 1>();

    [[maybe_unused]] static auto task0 = rtos_test::TaskWithStack(0, false, []{
        rtos_test::checkpoint(3);
        EXPECT(mqueue_int.dequeue() == 42);
        EXPECT(mqueue_int.dequeue() == -42);
        EXPECT(mqueue_custom_type.dequeue() == CustomType());
        rtos_test::pass();
    });

    [[maybe_unused]] static auto task1 = rtos_test::TaskWithStack(1, false, []{
        rtos_test::checkpoint(1);
        mqueue_int.enqueue(42);
        mqueue_int.enqueue(-42);
        mqueue_custom_type.enqueue(CustomType());
        rtos_test::checkpoint(2);
    });

    rtos::start();
}
