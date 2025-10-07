#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "termo.h"
#include "tim.h"
#include "usart.h"

static termo_config_t config = {
    .control_ctx = {
        .config = {.delta_timer = &htim2,
                   .mcp9808_i2c_bus = &hi2c1,
                   .mcp9808_i2c_address = MCP9808_SLAVE_ADDRESS_A2L_A1L_A0L}}};

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();
    MX_TIM2_Init();

    termo_initialize(&config);
}