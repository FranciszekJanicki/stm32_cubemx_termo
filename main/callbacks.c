#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_task.h"
#include "config.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM1) {
        HAL_IncTick();
    } else if (htim->Instance == TIM2) {
        termo_task_delta_timer_callback();
    }
}
