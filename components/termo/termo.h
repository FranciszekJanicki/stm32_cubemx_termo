#ifndef TERMO_TERMO_H
#define TERMO_TERMO_H

#include "control_task.h"
#include "display_task.h"
#include "system_task.h"
#include "termo_common.h"

typedef struct {
    system_task_ctx_t system_ctx;
    display_task_ctx_t display_ctx;
    control_task_ctx_t control_ctx;
} termo_config_t;

void termo_initialize(termo_config_t const* config);

#endif // TERMO_TERMO_H