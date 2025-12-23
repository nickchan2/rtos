#include "huart.h"
#include "rtos_test.hh"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"

#include <cstdlib>
#include <cstdio>
#include <optional>
#include <string_view>

namespace {

struct FailArgs {
    const char *msg;
    const char *file;
    int line;
};

struct CheckPointArgs {
    int num;
    const char *file;
    int line;
};

TIM_HandleTypeDef htim2;
std::optional<void(*)()> timer_callback;
bool hardfault_expected = false;

[[noreturn]] void test_finished() {
    puts("<Test finished>");
    while (true) {}
}

void tim2_init() {
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init = {
        .Prescaler          = 16000 - 1,
        .CounterMode        = TIM_COUNTERMODE_UP,
        .Period             = 1000 - 1,
        .ClockDivision      = TIM_CLOCKDIVISION_DIV1,
        .RepetitionCounter  = 0,
        .AutoReloadPreload  = TIM_AUTORELOAD_PRELOAD_DISABLE,
    };
    HAL_TIM_Base_Init(&htim2);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
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
    HAL_NVIC_SetPriority(TIM2_IRQn, 8, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 14, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0); // Lowest possible priority
}

[[noreturn, gnu::naked]] void test_failed_syscall(const FailArgs &args) {
    asm volatile("svc 130");
}

[[noreturn]] void test_failed(const FailArgs &args) {
    __disable_irq();
    std::printf("Fail: (%s:%d)\r\n", args.file, args.line);
    std::printf("%s\r\n", args.msg);
    test_finished();
}

void test_passed() {
    puts("Pass");
    test_finished();
}

[[gnu::naked]] void checkpoint_syscall(const CheckPointArgs &args) {
    asm volatile(
        "svc 128    \n"
        "bx lr      \n"
    );
}

void checkpoint(const CheckPointArgs &args) {
    static volatile int last = 0;
    if (args.num != last + 1) {
        std::array<char, 64> msg;
        std::snprintf(msg.data(),
                      msg.size(),
                      "Checkpoint %d was reached when %d was expected",
                      args.num,
                      last + 1);
        test_failed({msg.data(), args.file, args.line});
    }
    last = last + 1;
}

} // namespace

void rtos_test::setup() {
    HAL_Init();
    uart_init();
    tim2_init();
    configure_nvic_for_rtos();
}

void rtos_test::set_timer_callback(void (*callback)()) {
    timer_callback = callback;
}

void rtos_test::start_timer() {
    HAL_TIM_Base_Start_IT(&htim2);
}

void rtos_test::checkpoint(int num, std::source_location location) {
    checkpoint_syscall({
        num,
        location.file_name(),
        static_cast<int>(location.line())
    });
}

[[gnu::naked]] void rtos_test::pass() {
    asm volatile("svc 129");
}

void rtos_test::expect_hardfault_to_pass(void (*func)()) {
    hardfault_expected = true;
    func();
    fail("Expected hardfault");
}

void rtos_test::fail(std::string_view msg, std::source_location location) {
    test_failed_syscall({
        msg.data(),
        location.file_name(),
        static_cast<int>(location.line())
    });
}

extern "C" {

// RTOS hooks

void rtos_failed_assert(const char *cond, const char *file, int line) {
    __disable_irq();
    printf("Assertion fail: %s (%s:%d)\r\n", cond, file, line);
    test_finished();
}

void rtos_failed_usage_assert(const char *cond, const char *file, int line,
                              const char *msg)
{
    __disable_irq();
    std::printf("RTOS usage assertion fail: (%s:%d)\r\n", file, line);
    std::printf("%s\r\n", msg);
    test_finished();
}

size_t debug_syscall(void *arg, int svc_num) {
    __disable_irq();
    switch (svc_num) {
        case 128:
            checkpoint(*reinterpret_cast<CheckPointArgs *>(arg));
            break;
        case 129:
            test_passed();
            break;
        case 130:
            test_failed(*reinterpret_cast<FailArgs *>(arg));
            break;
        default:
            break;
    }
    __enable_irq();
    return 0;
}

// Interrupt handlers

void SysTick_Handler(void) {
    HAL_IncTick();
    rtos::tick();
}

void TIM2_IRQHandler(void) {
    if (timer_callback.has_value()) {
        timer_callback.value()();
    }
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
}

#define FAULT_HANDLER(fault_name) \
    void fault_name##_Handler() { \
        puts(#fault_name); \
        test_finished(); \
    }

FAULT_HANDLER(MemManage)
FAULT_HANDLER(UsageFault)

void HardFault_Handler() {
    if (hardfault_expected) {
        test_passed();
    } else {
        puts("HardFault");
        test_finished();
    }
}

} // extern "C"
