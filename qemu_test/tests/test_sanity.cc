#include "rtos_test.hh"

int main() {
    rtos_test::setup();
    CHECKPOINT(1);
    CHECKPOINT(2);
    rtos_test::pass();
}
