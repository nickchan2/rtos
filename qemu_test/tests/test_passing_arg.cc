#include "rtos_test.hh"

int main(void) {
    quick_setup();
    Task task(1, (void *)0xDEADBEEFU, [](void *arg) {
        assert((size_t)arg == 0xDEADBEEFU);
        test_passed();
    });
    rtos_start();
}
