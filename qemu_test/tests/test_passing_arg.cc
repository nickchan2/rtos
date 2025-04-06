#include "rtos_test.hh"

static void *const passed = reinterpret_cast<void *>(0xDEADBEEFU);

int main(void) {
    rtos_test::setup();
    rtos::TaskWithStack task(1, passed, [](void *arg) {
        EXPECT(arg == passed);
        rtos_test::pass();
    });
    rtos::start();
}
