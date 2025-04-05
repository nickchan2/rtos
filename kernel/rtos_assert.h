#pragma once

#include "cortex_m4.h"
#include "rtos.h"

#if RTOS_ENABLE_USAGE_ASSERT

#define USAGE_ASSERT(cond, msg) \
    if (!(cond)) rtos_failed_usage_assert(#cond, __FILE__, __LINE__, msg)

[[noreturn]] void rtos_failed_usage_assert(
    const char *cond, const char *file, int line, const char *msg);

#else // #if RTOS_ENABLE_USAGE_ASSERT

#define USAGE_ASSERT(cond, msg)

#endif // #if RTOS_ENABLE_USAGE_ASSERT

#if RTOS_DEBUG

#define ASSERT(cond) \
    if (!(cond)) rtos_failed_assert(#cond, __FILE__, __LINE__)

[[noreturn]] void rtos_failed_assert(
    const char *cond, const char *file, int line);

#else // #if RTOS_DEBUG

#define ASSERT(cond)

#endif // #if RTOS_DEBUG
