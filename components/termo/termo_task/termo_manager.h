#ifndef TERMO_TASK_TERMO_MANAGER_H
#define TERMO_TASK_TERMO_MANAGER_H

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
} termo_config_t;

typedef struct {
    float kp;
    float ki;
    float kd;
    float kc;
    float min_temp;
    float max_temp;
    float sampling_time;
} termo_params_t;

typedef struct {
    bool is_running;
    float reference;
    float measurement;
    float delta_time;

    mcp9808_t mcp9808;
    pid_regulator_t pid;
    termo_config_t config;
} termo_manager_t;

termo_err_t termo_manager_process(termo_manager_t* manager);
termo_err_t termo_manager_initialize(termo_manager_t* manager,
                                     termo_config_t const* config,
                                     termo_params_t const* params);

#endif // TERMO_TASK_TERMO_MANAGER_H