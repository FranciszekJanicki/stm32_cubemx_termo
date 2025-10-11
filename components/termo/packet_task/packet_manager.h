#ifndef PACKET_TASK_PACKET_MANAGER_H
#define PACKET_TASK_PACKET_MANAGER_H

#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_common.h"
#include <stdint.h>

typedef struct {
    
} packet_config_t;

typedef struct {
    bool is_running;

    packet_config_t config;
} packet_manager_t;

termo_err_t packet_manager_process(packet_manager_t* manager);
termo_err_t packet_manager_initialize(packet_manager_t* manager,
                                      packet_config_t const* config);

#endif // PACKET_TASK_PACKET_MANAGER_H