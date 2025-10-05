#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "termo.h"
#include "usart.h"

static termo_config_t config = {};

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_I2C1_Init();

    termo_initialize(&config);
}