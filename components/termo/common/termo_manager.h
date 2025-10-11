#include "FreeRTOS.h"
#include "handle_manager.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

typedef enum {
    TERMO_TASK_TYPE_SYSTEM,
    TERMO_TASK_TYPE_CONTROL,
    TERMO_TASK_TYPE_DISPLAY,
    TERMO_TASK_TYPE_PACKET,
    TERMO_TASK_TYPE_NUM,
} termo_task_type_t;

typedef enum {
    TERMO_QUEUE_TYPE_SYSTEM,
    TERMO_QUEUE_TYPE_CONTROL,
    TERMO_QUEUE_TYPE_DISPLAY,
    TERMO_QUEUE_TYPE_PACKET,
    TERMO_QUEUE_TYPE_NUM,
} termo_queue_type_t;

typedef enum {
    TERMO_SEMAPHORE_TYPE_LOG,
    TERMO_SEMAPHORE_TYPE_NUM,
} termo_semaphore_type_t;

DECLARE_HANDLE_MANAGER(termo_task,
                       termo_task_type_t,
                       TaskHandle_t,
                       TERMO_TASK_TYPE_NUM);

DECLARE_HANDLE_MANAGER(termo_queue,
                       termo_queue_type_t,
                       QueueHandle_t,
                       TERMO_QUEUE_TYPE_NUM);

DECLARE_HANDLE_MANAGER(termo_semaphore,
                       termo_semaphore_type_t,
                       SemaphoreHandle_t,
                       TERMO_SEMAPHORE_TYPE_NUM);