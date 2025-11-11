#ifndef MAIN_CONFIG_H
#define MAIN_CONFIG_H

#include "i2c.h"
#include "main.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

extern TIM_HandleTypeDef htim1;

#define SYSTICK_TIMER (&htim1)

#define DELTA_TIMER (&htim2)

#define UPDATE_TIMER (&htim4)

#define MCP9808_I2C_BUS (&hi2c1)
#define MCP9808_I2C_ADDRESS (MCP9808_SLAVE_ADDRESS_A2L_A1L_A0L)

#define PWM_TIMER (&htim3)
#define PWM_CHANNEL (TIM_CHANNEL_1)

#define PROP_GAIN (100.0F)
#define INT_GAIN (0.F)
#define DOT_GAIN (0.0F)
#define SAT_GAIN (0.0F)
#define MIN_TEMP (25.0F)
#define MAX_TEMP (35.0F)
#define MIN_COMPARE (0x0000U)
#define MAX_COMPARE (0xFFFFU)
#define DELTA_TIME (1.0F)

#define SH1107_SPI_BUS (&hspi1)
#define SH1107_CONTROL_GPIO (GPIOC)
#define SH1107_CONTROL_PIN (1U << 7U)
#define SH1107_RESET_GPIO (GPIOC)
#define SH1107_RESET_PIN (1U << 8U)
#define SH1107_SLAVE_SELECT_GPIO (GPIOC)
#define SH1107_SLAVE_SELECT_PIN (1U << 6U)

#define LOG_UART_BUS (&huart2)
#define PACKET_UART_BUS (&huart1)

#endif // MAIN_CONFIG_H