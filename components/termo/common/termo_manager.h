#include "FreeRTOS.h"
#include "handle_manager.h"
#include "task.h"

typedef enum {
    TERMO_TASK_TYPE_CONTROL,
    TERMO_TASK_TYPE_NUM,
} termo_task_type_t;

DECLARE_HANDLE_MANAGER(termo_task,
                       termo_task_type_t,
                       TaskHandle_t,
                       TERMO_TASK_TYPE_NUM);