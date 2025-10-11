#ifndef PACKET_TASK_PACKET_TASK_H
#define PACKET_TASK_PACKET_TASK_H

#include "packet_manager.h"
#include "termo_common.h"

typedef struct {
    packet_config_t config;
} packet_task_ctx_t;

termo_err_t packet_task_initialize(packet_task_ctx_t const* task_ctx);

#endif // PACKET_TASK_PACKET_TASK_H