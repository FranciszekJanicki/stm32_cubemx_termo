#ifndef TERMO_TERMO_H
#define TERMO_TERMO_H

#include "display_task.h"
#include "packet_task.h"
#include "system_task.h"
#include "termo_common.h"
#include "termo_task.h"

typedef struct {
    system_task_ctx_t system_ctx;
    display_task_ctx_t display_ctx;
    termo_task_ctx_t termo_ctx;
    packet_task_ctx_t packet_ctx;
} termo_ctx_t;

void termo_initialize(termo_ctx_t const* config);

#endif // TERMO_TERMO_H