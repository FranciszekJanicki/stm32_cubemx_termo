#ifndef SYSTEM_TASK_SYSTEM_TASK_H
#define SYSTEM_TASK_SYSTEM_TASK_H

#include "system_manager.h"
#include "termo_common.h"

typedef struct {
    system_config_t config;
} system_task_ctx_t;

termo_err_t system_task_initialize(system_task_ctx_t const* task_ctx);

#endif // SYSTEM_TASK_SYSTEM_TASK_H