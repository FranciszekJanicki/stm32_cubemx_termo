#ifndef SYSTEM_TASK_SYSTEM_MANAGER_H
#define SYSTEM_TASK_SYSTEM_MANAGER_H

#include "stm32f446xx.h"
#include "stm32f4xx_hal.h"
#include "termo_common.h"
#include <stdint.h>

typedef struct {
} system_config_t;

typedef struct {
    bool is_termo_running;
    bool is_display_running;
    bool is_packet_running;
    float reference_temperature;
    float measure_temperature;
    float measure_humidity;
    float measure_pressure;
    float sampling_time;

    system_config_t config;
} system_manager_t;

termo_err_t system_manager_process(system_manager_t* manager);
termo_err_t system_manager_initialize(system_manager_t* manager,
                                      system_config_t const* config);

#endif // SYSTEM_TASK_SYSTEM_MANAGER_H