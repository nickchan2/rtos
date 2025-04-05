#include "rtos_test.hh"

static void *const passed = reinterpret_cast<void *>(0xDEADBEEFU);

int main(void) {
    rtos_test::setup();
    rtos_test::TaskWithStack task(0, false, passed, [](void *arg) {
        EXPECT(arg == passed);
        rtos_test::pass();
    });
    rtos::start();
}
