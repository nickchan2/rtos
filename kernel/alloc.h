#pragma once

#include <stdlib.h>

static inline void *internal_malloc(size_t size)
{
    return malloc(size);
}

static inline void internal_free(void *ptr)
{
    free(ptr);
}
