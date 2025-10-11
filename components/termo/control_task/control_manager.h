#ifndef CONTROL_TASK_CONTROL_MANAGER_H
#define CONTROL_TASK_CONTROL_MANAGER_H

#include "mcp9808.h"
#include "pid_regulator.h"
#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_common.h"
#include <stdint.h>

typedef struct {
    I2C_HandleTypeDef* mcp9808_i2c_bus;
    uint16_t mcp9808_i2c_address;
    TIM_HandleTypeDef* delta_timer;
    TIM_HandleTypeDef* pwm_timer;
    uint16_t pwm_channel;
} control_config_t;

typedef struct {
    float kp;
    float ki;
    float kd;
    float kc;
    float min_temp;
    float max_temp;
    float sampling_time;
} control_params_t;

typedef struct {
    bool is_running;
    float reference;
    float measurement;
    float delta_time;

    mcp9808_t mcp9808;
    pid_regulator_t pid;
    control_config_t config;
} control_manager_t;

termo_err_t control_manager_process(control_manager_t* manager);
termo_err_t control_manager_initialize(control_manager_t* manager,
                                       control_config_t const* config,
                                       control_params_t const* params);

#endif // CONTROL_TASK_CONTROL_MANAGER_H