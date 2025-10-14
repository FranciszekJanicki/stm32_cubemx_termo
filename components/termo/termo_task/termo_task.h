#ifndef TERMO_TASK_TERMO_TASK_H
#define TERMO_TASK_TERMO_TASK_H

#include "termo_common.h"
#include "termo_manager.h"

typedef struct {
    termo_config_t config;
    termo_params_t params;
} termo_task_ctx_t;

termo_err_t termo_task_initialize(termo_task_ctx_t const* task_ctx);

void termo_task_delta_timer_callback(void);

#endif // TERMO_TASK_TERMO_TASK_H