#ifndef CONTROL_TASK_CONTROL_MANAGER_H
#define CONTROL_TASK_CONTROL_MANAGER_H

#include "mcp9808.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_common.h"
#include <stdint.h>

typedef struct {
    I2C_HandleTypeDef* mcp9808_i2c_bus;
    uint16_t mcp9808_i2c_address;
    TIM_HandleTypeDef* delta_timer;
} control_config_t;

typedef struct {
    mcp9808_t mcp9808;
    control_config_t config;

    bool is_running;
    float reference;
    float measurement;
    float delta_time;
} control_manager_t;

termo_err_t control_manager_process(control_manager_t* manager);
termo_err_t control_manager_initialize(control_manager_t* manager,
                                       control_config_t const* config);

#endif // CONTROL_TASK_CONTROL_MANAGER_H