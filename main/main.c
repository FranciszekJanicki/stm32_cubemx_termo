#include "main.h"
#include "config.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "termo.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"

static termo_ctx_t config = {
    .system_ctx = {.config = {}},
    .termo_ctx = {.config = {.delta_timer = DELTA_TIMER,
                             .mcp9808_i2c_bus = MCP9808_I2C_BUS,
                             .mcp9808_i2c_address = MCP9808_I2C_ADDRESS,
                             .pwm_timer = PWM_TIMER,
                             .pwm_channel = PWM_CHANNEL},
                  .params = {.kp = PROP_GAIN,
                             .ki = INT_GAIN,
                             .kd = DOT_GAIN,
                             .kc = SAT_GAIN,
                             .min_temp = MIN_CONTROL,
                             .max_temp = MAX_CONTROL,
                             .sampling_time = SAMPLING_TIME}},
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
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_SPI3_Init();
    MX_USB_DEVICE_Init();

    HAL_Delay(100U);

    termo_initialize(&config);
}