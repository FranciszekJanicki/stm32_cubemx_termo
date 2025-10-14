#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "spi.h"
#include "termo.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"

static termo_ctx_t config = {
    .system_ctx = {.config = {}},
    .termo_ctx = {.config = {.delta_timer = &htim2,
                             .mcp9808_i2c_bus = &hi2c1,
                             .mcp9808_i2c_address =
                                 MCP9808_SLAVE_ADDRESS_A2L_A1L_A0L,
                             .pwm_timer = &htim3,
                             .pwm_channel = TIM_CHANNEL_2},
                  .params = {.kp = 10.0F,
                             .ki = 0.5F,
                             .kd = 1.0F,
                             .kc = 0.1F,
                             .min_temp = 0.0F,
                             .max_temp = 100.0F,
                             .sampling_time = 1.0F}},
    .display_ctx = {
        .config = {.sh1107_spi_bus = &hspi3,
                   .sh1107_control_gpio = SH1107_CONTROL_GPIO_Port,
                   .sh1107_control_pin = SH1107_CONTROL_Pin,
                   .sh1107_reset_gpio = SH1107_RESET_GPIO_Port,
                   .sh1107_reset_pin = SH1107_RESET_Pin,
                   .sh1107_slave_select_gpio = SH1107_SLAVE_SELECT_GPIO_Port,
                   .sh1107_slave_select_pin = SH1107_SLAVE_SELECT_Pin}}};

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