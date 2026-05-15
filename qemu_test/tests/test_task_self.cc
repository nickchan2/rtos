#include "rtos_test.hh"

#include <optional>

namespace {
std::optional<rtos_test::TaskWithStack<>> task;
} // namespace

auto main() -> int {
    rtos_test::setup();
    task.emplace(0, false, []{
        EXPECT(&rtos::Task::self() == &task.value().task);
        rtos_test::pass();
    });
    rtos::start();
}
