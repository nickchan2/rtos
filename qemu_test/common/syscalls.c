#include "huart.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

[[gnu::used]] int _getpid(void) {
    return 1;
}

[[gnu::used]] int _kill(int pid, int sig) {
    return -1;
}

void _exit(int status) {
    __disable_irq();
    while (true) {
        __asm volatile("wfi");
    }
}

void *_sbrk(ptrdiff_t incr) {
    extern char _end; // Start of heap space
    extern char _estack; // End of heap space
    static size_t heap_end = (size_t)&_end;

    const size_t prev_heap_end = heap_end;
    if (heap_end + incr > (size_t)&_estack) {
        return (void *)-1;
    }
    heap_end += incr;
    return (void *)prev_heap_end;
}

int _close(int file) {
    return -1;
}

int _read(int file, char *ptr, int len) {
    HAL_UART_Receive(&huart, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

int _lseek(int file, int ptr, int dir) {
    return 0;
}

int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

[[gnu::used]] int _fstat(int file, struct stat *st) {
    return 0;
}

[[gnu::used]] int _isatty(int file) {
    return 1;
}
