#ifndef DISPLAY_TASK_DISPLAY_MANAGER_H
#define DISPLAY_TASK_DISPLAY_MANAGER_H

#include "font5x7.h"
#include "sh1107.h"
#include "stm32f446xx.h"
#include "stm32f4xx_hal.h"
#include "termo_common.h"
#include <stdint.h>

typedef struct {
    SPI_HandleTypeDef* sh1107_spi_bus;
    GPIO_TypeDef* sh1107_slave_select_gpio;
    uint16_t sh1107_slave_select_pin;
    GPIO_TypeDef* sh1107_control_gpio;
    uint16_t sh1107_control_pin;
    GPIO_TypeDef* sh1107_reset_gpio;
    uint16_t sh1107_reset_pin;
} display_config_t;

typedef struct {
    bool is_running;
    display_config_t config;

    sh1107_t sh1107;
    uint8_t sh1107_frame_buffer[SH1107_FRAME_BUFFER_SIZE];

    float reference_temperature;
    float sampling_time;

    float measure_temperature;
    float measure_pressure;
    float measure_humidity;
} display_manager_t;

termo_err_t display_manager_process(display_manager_t* manager);
termo_err_t display_manager_initialize(display_manager_t* manager,
                                       display_config_t const* config);

#endif // DISPLAY_TASK_DISPLAY_MANAGER_H
