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
                             .pwm_timer = PWM_TIMER,
                             .pwm_channel = PWM_CHANNEL},
                  .params = {.kp = PROP_GAIN,
                             .ki = INT_GAIN,
                             .kd = DOT_GAIN,
                             .kc = SAT_GAIN,
                             .min_temp = MIN_CONTROL,
                             .max_temp = MAX_CONTROL,
                             .sampling_time = SAMPLING_TIME}},
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
    MX_SPI1_Init();

    HAL_Delay(100U);

    // stdin->USART1 RX stdout->USART2 TX stderr->USART1 TX

    HAL_TIM_PWM_Start(PWM_TIMER, PWM_CHANNEL);
    __HAL_TIM_SET_COMPARE(PWM_TIMER, PWM_CHANNEL, (uint16_t)(65535.0F / 2.0F));

    // char read_buffer[100];
    // char write_buffer[100];
    // while (1) {
    //     memset(write_buffer, 0xFF, sizeof(write_buffer));

    //     strncpy(write_buffer + sizeof(write_buffer) - strlen("\n\r") - 1,
    //             "\n\r",
    //             strlen("\n\r"));
    //     write_buffer[sizeof(write_buffer) - 1] = '\0';
    //     fwrite(write_buffer,
    //            sizeof(*write_buffer),
    //            sizeof(write_buffer),
    //            stderr);

    //     fread(read_buffer,
    //           sizeof(*read_buffer),
    //           sizeof(read_buffer) - 1,
    //           stdin);

    //     strncpy(read_buffer + sizeof(read_buffer) - strlen("\n\r") - 1,
    //             "\n\r",
    //             strlen("\n\r"));
    //     read_buffer[sizeof(read_buffer) - 1] = '\0';
    //     fwrite(read_buffer,
    //            sizeof(*read_buffer),
    //            sizeof(read_buffer) - 1,
    //            stdout);

    //     memset(write_buffer, 0, sizeof(write_buffer));
    //     memset(read_buffer, 0, sizeof(read_buffer));
    // }

    termo_initialize(&config);
}