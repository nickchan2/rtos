#include "huart.h"
#include "rtos_test.hh"
#include "stm32f4xx_hal.h"

#include <cstdlib>
#include <cstdio>

namespace {

[[noreturn]] void test_finished() {
    puts("<Test finished>\r\n");
    exit(0);
}

void uart_init() {
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

void configure_nvic_for_rtos() {
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(SVCall_IRQn, 7, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 14, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0); // Lowest possible priority
}

} // namespace

void rtos_test::setup() {
    HAL_Init();
    uart_init();
    configure_nvic_for_rtos();
}

void rtos_test::pass() {
    __disable_irq();
    printf("Pass\r\n");
    test_finished();
}

void rtos_test::checkpoint(int num, const char *file, int line) {
    static volatile int last = 0;
    if (num != last + 1) {
        __disable_irq();
        printf("Checkoint fail: (%s:%d)\r\n", file, line);
        printf("%d was reached when %d was expected\r\n", num, last + 1);
        test_finished();
    }
    ++last;
}

void rtos_test::fail(const char *msg, const char *file, int line) {
    __disable_irq();
    printf("Fail: (%s:%d)\r\n", file, line);
    printf("%s\r\n", msg);
    test_finished();
}

extern "C" void rtos_failed_assert(const char *cond, const char *file,
                                   int line, const char *msg) {
    __disable_irq();
    printf("RTOS usage assertion fail: (%s:%d)\r\n", file, line);
    printf("%s\r\n", msg);
    test_finished();
}

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
    rtos::tick();
}
