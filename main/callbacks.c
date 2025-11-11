#include "config.h"
#include "packet_task.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_task.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == SYSTICK_TIMER->Instance) {
        HAL_IncTick();
    } else if (htim->Instance == DELTA_TIMER->Instance) {
        termo_task_delta_timer_callback();
    } else if (htim->Instance == UPDATE_TIMER->Instance) {
        termo_task_update_timer_callback();
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == PWM_TIMER->Instance) {
        termo_task_pwm_timer_callback();
    }
}

void HAL_UART_RxCplt_Callback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == PACKET_UART_BUS->Instance) {
        packet_task_rx_complete_callback();
    }
}
