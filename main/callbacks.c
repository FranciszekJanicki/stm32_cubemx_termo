#include "config.h"
#include "packet_task.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_task.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM1) {
        HAL_IncTick();
    } else if (htim->Instance == TIM2) {
        termo_task_delta_timer_callback();
    }
}

void HAL_UART_RxCplt_Callback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == PACKET_UART_BUS->Instance) {
        packet_task_rx_complete_callback();
    }
}
