#include "rtos.hh"
#include "rtos_test.hh"

int main() {
    rtos_test::setup();
    rtos::TaskWithStack task(0, []{ rtos_test::pass(); });
    rtos::start();
}
