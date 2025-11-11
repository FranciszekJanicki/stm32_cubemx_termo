#include "main.h"
#include "config.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "termo.h"
#include "tim.h"
#include "usart.h"
#include <locale.h>
#include <string.h>

static termo_ctx_t config = {
    .system_ctx = {.config = {}},
    .termo_ctx = {.config = {.delta_timer = DELTA_TIMER,
                             .mcp9808_i2c_bus = MCP9808_I2C_BUS,
                             .mcp9808_i2c_address = MCP9808_I2C_ADDRESS,
                             .update_timer = UPDATE_TIMER,
                             .pwm_timer = PWM_TIMER,
                             .pwm_channel = PWM_CHANNEL},
                  .params = {.kp = PROP_GAIN,
                             .ki = INT_GAIN,
                             .kd = DOT_GAIN,
                             .kc = SAT_GAIN,
                             .min_temp = MIN_TEMP,
                             .max_temp = MAX_TEMP,
                             .min_compare = MIN_COMPARE,
                             .max_compare = MAX_COMPARE,
                             .delta_time = DELTA_TIME}},
    .packet_ctx = {.config = {.packet_uart_bus = PACKET_UART_BUS}},
    .display_ctx = {
        .config = {.sh1107_spi_bus = SH1107_SPI_BUS,
                   .sh1107_control_gpio = SH1107_CONTROL_GPIO,
                   .sh1107_control_pin = SH1107_CONTROL_PIN,
                   .sh1107_reset_gpio = SH1107_RESET_GPIO,
                   .sh1107_reset_pin = SH1107_RESET_PIN,
                   .sh1107_slave_select_gpio = SH1107_SLAVE_SELECT_GPIO,
                   .sh1107_slave_select_pin = SH1107_SLAVE_SELECT_PIN}}};

void SystemClock_Config(void);

int main(void)
{
    setlocale(LC_NUMERIC, "C");

    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_TIM4_Init();
    MX_SPI1_Init();

    HAL_Delay(100U);

    termo_initialize(&config);
}