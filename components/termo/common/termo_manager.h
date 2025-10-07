#include "FreeRTOS.h"
#include "handle_manager.h"
#include "queue.h"
#include "task.h"

typedef enum {
    TERMO_TASK_TYPE_CONTROL,
    TERMO_TASK_TYPE_NUM,
} termo_task_type_t;

typedef enum {
    TERMO_QUEUE_TYPE_CONTROL,
    TERMO_QUEUE_TYPE_NUM,
} termo_queue_type_t;

DECLARE_HANDLE_MANAGER(termo_task,
                       termo_task_type_t,
                       TaskHandle_t,
                       TERMO_TASK_TYPE_NUM);

DECLARE_HANDLE_MANAGER(termo_queue,
                       termo_queue_type_t,
                       QueueHandle_t,
                       TERMO_QUEUE_TYPE_NUM);
