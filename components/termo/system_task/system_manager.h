#ifndef SYSTEM_TASK_SYSTEM_MANAGER_H
#define SYSTEM_TASK_SYSTEM_MANAGER_H

#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_common.h"
#include <stdint.h>

typedef struct {
} system_config_t;

typedef struct {
    system_config_t config;
} system_manager_t;

termo_err_t system_manager_process(system_manager_t* manager);
termo_err_t system_manager_initialize(system_manager_t* manager,
                                      system_config_t const* config);

#endif // SYSTEM_TASK_SYSTEM_MANAGER_H