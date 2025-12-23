#include "rtos_test.hh"

int main() {
    rtos_test::setup();
    rtos_test::checkpoint(1);
    rtos_test::checkpoint(2);
    rtos_test::pass();
}
