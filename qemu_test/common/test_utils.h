#pragma once

#include "stm32f4xx_hal.h" // IWYU pragma: export

#include <stdint.h>
#include <stdnoreturn.h>

typedef struct {
    uint8_t data[512] __attribute__((aligned(8)));
} stack_512_t;

extern UART_HandleTypeDef huart;

void quick_setup(void);

void uart_init(void);

void configure_nvic_for_rtos(void);

noreturn void test_passed(void);
