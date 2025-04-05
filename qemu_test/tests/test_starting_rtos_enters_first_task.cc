#include "rtos_test.hh"

int main() {
    quick_setup();
    Task task(1, []{ test_passed(); });
    rtos_start();
}
