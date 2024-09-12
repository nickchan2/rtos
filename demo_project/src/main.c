#include <rtos.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stm32f4xx_hal.h>

static UART_HandleTypeDef huart2;
static TIM_HandleTypeDef htim2;
static struct rtos_mutex *uart_mutex_handle;
static struct rtos_task *button_task_handle;
static struct rtos_task *timer_task_handle;
static struct rtos_task *led_task_handle;
static volatile size_t work_count[2];

static void tim2_init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();
    
    htim2.Instance = TIM2;
    htim2.Init.Prescaler            = 16000 - 1; /* 1 ms tick */
    htim2.Init.CounterMode          = TIM_COUNTERMODE_UP;
    htim2.Init.Period               = 1000 - 1; /* 1 s period */
    htim2.Init.ClockDivision        = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload    = TIM_AUTORELOAD_PRELOAD_ENABLE;

    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    HAL_TIM_Base_Init(&htim2);
}

static void uart2_init(void)
{
    __USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Rx pin */
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
        .Pin        = GPIO_PIN_2,
        .Mode       = GPIO_MODE_AF_PP,
        .Pull       = GPIO_NOPULL,
        .Speed      = GPIO_SPEED_HIGH,
        .Alternate  = GPIO_AF7_USART2,
    });

    /* Tx pin */
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
        .Pin        = GPIO_PIN_3,
        .Mode       = GPIO_MODE_AF_OD,
        .Pull       = GPIO_NOPULL,
        .Speed      = GPIO_SPEED_HIGH,
        .Alternate  = GPIO_AF7_USART2,
    });

    huart2.Instance        = USART2;
    huart2.Init.BaudRate   = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits   = UART_STOPBITS_1;
    huart2.Init.Parity     = UART_PARITY_NONE;
    huart2.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    huart2.Init.Mode       = UART_MODE_TX_RX;
    HAL_UART_Init(&huart2);
}

static void other_gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* LD2 Pin */
    HAL_GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
        .Pin    = GPIO_PIN_5,
        .Mode   = GPIO_MODE_OUTPUT_PP,
        .Pull   = GPIO_NOPULL,
        .Speed  = GPIO_SPEED_LOW,
    });

    /* B1 Pin */
    HAL_GPIO_Init(GPIOC, &(GPIO_InitTypeDef){
        .Pin    = GPIO_PIN_13,
        .Mode   = GPIO_MODE_IT_FALLING,
        .Pull   = GPIO_NOPULL,
        .Speed  = GPIO_SPEED_LOW,
    });
    
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* Transmit a string using UART2 (FIXME: HAL_UART_Transmit isn't working) */
static void uart_transmit_str(const char *str)
{
    while (*str != '\0') {
        while ((huart2.Instance->SR & USART_SR_TXE) == 0) {}
        huart2.Instance->DR = *str++;
    }
}

static void timer_task(void *arg)
{
    while (true) {
        rtos_task_suspend();
        rtos_mutex_lock(uart_mutex_handle);
        uart_transmit_str("Timer expired\n");
        rtos_mutex_unlock(uart_mutex_handle);
    }
}

static void button_task(void *arg)
{
    while (true) {
        rtos_task_suspend();
        char buf[64];
        snprintf(buf, sizeof(buf), "Button pressed %u %u\n", work_count[0],
                 work_count[1]);
        rtos_mutex_lock(uart_mutex_handle);
        uart_transmit_str(buf);
        rtos_mutex_unlock(uart_mutex_handle);
    }
}

static void worker_task(void *arg)
{
    size_t *const work_count = arg;
    while (true) {
        volatile float x = 0;
        for (size_t i = 0; i < 1000000; i++) {
            x += i;
        }
        ++(*work_count);
        rtos_task_sleep(10);
    }
}

static void led_task(void *arg)
{
    while (true) {
        rtos_task_suspend();
        for (int i = 0; i < 4; ++i) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            rtos_task_sleep(100);
        }
    }
}

int main(void)
{
    HAL_Init();
    uart2_init();
    tim2_init();
    other_gpio_init();

    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(SVCall_IRQn, 7, 0);
    HAL_NVIC_SetPriority(TIM2_IRQn, 8, 0);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 8, 0);
    HAL_NVIC_SetPriority(SysTick_IRQn, 14, 0);
    HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0); /* Lowest possible priority */

    HAL_TIM_Base_Start_IT(&htim2);

    uart_mutex_handle = rtos_mutex_create();

    button_task_handle = rtos_task_create(button_task, NULL, 2048, 2);
    timer_task_handle = rtos_task_create(timer_task, NULL, 2048, 2);
    led_task_handle = rtos_task_create(led_task, NULL, 2048, 3);
    rtos_task_create(worker_task, (void *)&work_count[0], 2048, 1);
    rtos_task_create(worker_task, (void *)&work_count[1], 2048, 1);

    /* Should never return. */
    rtos_start();

    return 0;
}

/* IRQ Handlers */

void SysTick_Handler(void)
{
    rtos_tick();
    HAL_IncTick();
}

void TIM2_IRQHandler(void)
{
    rtos_task_resume_from_isr(timer_task_handle);
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
}

void EXTI15_10_IRQHandler(void)
{
    rtos_task_resume_from_isr(button_task_handle);
    rtos_task_resume_from_isr(led_task_handle);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);
}

void HardFault_Handler(void)
{
    while (1) {}
}
