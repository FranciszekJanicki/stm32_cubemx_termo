#ifndef PACKET_TASK_PACKET_MANAGER_H
#define PACKET_TASK_PACKET_MANAGER_H

#include "stm32l476xx.h"
#include "stm32l4xx_hal.h"
#include "termo_common.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    UART_HandleTypeDef* packet_uart_bus;
} packet_config_t;

#define TRANSMIT_BUFFER_SIZE (150U)
#define RECEIVE_BUFFER_SIZE (150U)

typedef struct {
    bool is_running;
    bool is_transmit_pending;
    bool is_receive_pending;

    uint8_t transmit_buffer[TRANSMIT_BUFFER_SIZE];
    uint8_t receive_buffer[RECEIVE_BUFFER_SIZE];

    packet_config_t config;
} packet_manager_t;

#undef TRANSMIT_BUFFER_SIZE
#undef RECEIVE_BUFFER_SIZE

termo_err_t packet_manager_process(packet_manager_t* manager);
termo_err_t packet_manager_initialize(packet_manager_t* manager,
                                      packet_config_t const* config);

#endif // PACKET_TASK_PACKET_MANAGER_H