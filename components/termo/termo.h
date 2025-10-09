#ifndef TERMO_TERMO_H
#define TERMO_TERMO_H

#include "control_task.h"
#include "termo_common.h"
#include "display_task.h"

typedef struct {
    display_task_ctx_t display_ctx;
    control_task_ctx_t control_ctx;
} termo_config_t;

void termo_initialize(termo_config_t const* config);

#endif // TERMO_TERMO_H