#include "rtos.h"
#include "rtos.hh"
#include "rtos_test.hh"

#include <optional>

static TIM_HandleTypeDef htim2;
static std::optional<rtos::TaskWithStack<>> timer_task;

static void tim2_init() {
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init = {
        .Prescaler          = 16000 - 1, // 1 ms tick
        .CounterMode        = TIM_COUNTERMODE_UP,
        .Period             = 1000 - 1, // 1 s period
        .ClockDivision      = TIM_CLOCKDIVISION_DIV1,
        .RepetitionCounter  = 0,
        .AutoReloadPreload  = TIM_AUTORELOAD_PRELOAD_ENABLE,
    };
    HAL_TIM_Base_Init(&htim2);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
    HAL_NVIC_SetPriority(TIM2_IRQn, 8, 0);
    HAL_TIM_Base_Start_IT(&htim2);
}

int main() {
    rtos_test::setup();
    tim2_init();

    timer_task.emplace(0, []{
        CHECKPOINT(1);
        rtos::task::suspend();
        CHECKPOINT(3);
        rtos_test::pass();
    });

    rtos::start();
}

extern "C" void TIM2_IRQHandler(void) {
    if (timer_task.has_value()) {
        CHECKPOINT(2);
        rtos_task_resume_from_isr(&timer_task.value());
    }
    __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
}
