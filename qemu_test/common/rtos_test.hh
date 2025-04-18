#pragma once

#include "rtos.h" // IWYU pragma: export
#include "rtos.hh" // IWYU pragma: export
#include "stm32f4xx_hal.h" // IWYU pragma: export

#include <cstddef>

#define FAIL(msg) rtos_test::fail(msg, __FILE__, __LINE__)

#define EXPECT(cond) \
    do { \
        if (!(cond)) { \
            FAIL("Expected " #cond); \
        } \
    } while (false)

#define CHECKPOINT(num) rtos_test::checkpoint(num, __FILE__, __LINE__)

namespace rtos_test {

void setup();

void pass();

void fail(const char *msg, const char *file, int line);

void checkpoint(int num, const char *file, int line);

} // namespace rtos_test

extern "C" [[gnu::used]] inline void rtos_test_pass_asm() {
    rtos_test::pass();
}

extern "C" [[gnu::used]] inline void rtos_test_fail_asm() {
    rtos_test::fail("Failed in asm", "<unknown>", 0);
}
