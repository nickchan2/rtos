#include "rtos_test.hh"

auto main() -> int {
    rtos_test::setup();
    rtos_test::checkpoint(1);
    rtos_test::checkpoint(2);
    rtos_test::pass();
}
