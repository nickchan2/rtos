#include "rtos_test.hh"

namespace {

void *const passed = reinterpret_cast<void *>(0xDEADBEEFU);

} // namespace

auto main() -> int {
    rtos_test::setup();
    [[maybe_unused]] static auto task = rtos_test::TaskWithStack(0, false, passed, [](void *arg) {
        EXPECT(arg == passed);
        rtos_test::pass();
    });
    rtos::start();
}
