#ifndef DISPLAY_TASK_DISPLAY_TASK_H
#define DISPLAY_TASK_DISPLAY_TASK_H

#include "display_manager.h"
#include "termo_common.h"

typedef struct {
    display_config_t config;
} display_task_ctx_t;

termo_err_t display_task_initialize(display_task_ctx_t const* task_ctx);

#endif // DISPLAY_TASK_DISPLAY_TASK_H