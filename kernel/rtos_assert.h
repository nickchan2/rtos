#pragma once

#include "cortex_m4.h"
#include "rtos.h"

#if RTOS_ENABLE_USAGE_ASSERT

#define USAGE_ASSERT(cond, msg) \
    if (!(cond)) rtos_failed_assert(#cond, __FILE__, __LINE__, msg)

#define ASSERT(cond) \
    if (!(cond)) rtos_failed_assert(#cond, __FILE__, __LINE__, "Assertion failed")

[[gnu::weak, gnu::noinline]] void rtos_failed_assert(
    const char *cond, const char *file, int line, const char *msg)
{
    cm4_disable_irq();
    while (true) {}
}

#else // #if RTOS_ENABLE_USAGE_ASSERT

#define USAGE_ASSERT(cond, msg)

#endif // #if RTOS_ENABLE_USAGE_ASSERT
