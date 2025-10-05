#ifndef TERMO_TERMO_H
#define TERMO_TERMO_H

#include "control_task.h"
#include "termo_common.h"

typedef struct {
    control_task_ctx_t control_ctx;
} termo_config_t;

void termo_initialize(termo_config_t const* config);

#endif // TERMO_TERMO_H