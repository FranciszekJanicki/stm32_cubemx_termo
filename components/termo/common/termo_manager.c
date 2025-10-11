#include "termo_manager.h"

DEFINE_HANDLE_MANAGER(termo_task,
                      termo_task_type_t,
                      TaskHandle_t,
                      TERMO_TASK_TYPE_NUM);

DEFINE_HANDLE_MANAGER(termo_queue,
                      termo_queue_type_t,
                      QueueHandle_t,
                      TERMO_QUEUE_TYPE_NUM);

DEFINE_HANDLE_MANAGER(termo_semaphore,
                      termo_semaphore_type_t,
                      SemaphoreHandle_t,
                      TERMO_SEMAPHORE_TYPE_NUM);