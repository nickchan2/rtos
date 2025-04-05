#include "rtos.h"
#include "stm32f4xx_hal.h"
#include "test_utils.h"

#include <stdio.h>
#include <stdlib.h>

UART_HandleTypeDef huart;

void quick_setup(void)
{
    HAL_Init();
    uart_init();
    configure_nvic_for_rtos();
}

void uart_init(void)
{
    __USART2_CLK_ENABLE();
    huart.Instance          = USART1;
    huart.Init.BaudRate     = 115200;
    huart.Init.WordLength   = UART_WORDLENGTH_8B;
    huart.Init.StopBits     = UART_STOPBITS_1;
    huart.Init.Parity       = UART_PARITY_NONE;
    huart.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart.Init.Mode         = UART_MODE_TX_RX;
    HAL_UART_Init(&huart);
}

void configure_nvic_for_rtos(void)
{
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(SVCall_IRQn, 7, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 14, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0); // Lowest possible priority
}

void test_passed(void)
{
    printf("Pass\r\n");
    exit(0);
}

void SysTick_Handler(void)
{
    HAL_IncTick();
    rtos_tick();
}
