#include "rtos.hh"
#include "rtos_test.hh"

int main() {
    rtos_test::setup();
    rtos_test::TaskWithStack task(0, false, []{ rtos_test::pass(); });
    rtos::start();
}
