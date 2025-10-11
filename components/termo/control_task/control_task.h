#ifndef CONTROL_TASK_CONTROL_TASK_H
#define CONTROL_TASK_CONTROL_TASK_H

#include "control_manager.h"
#include "termo_common.h"

typedef struct {
    control_config_t config;
} control_task_ctx_t;

termo_err_t control_task_initialize(control_task_ctx_t const* task_ctx);

void control_task_delta_timer_callback(void);

#endif // CONTROL_TASK_CONTROL_TASK_H