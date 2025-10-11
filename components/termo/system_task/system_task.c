
#include "system_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"

#define SYSTEM_TASK_STACK_DEPTH (5000UL / sizeof(StackType_t))
#define SYSTEM_TASK_NAME ("system_task")
#define SYSTEM_TASK_PRIORITY (1UL)

#define SYSTEM_QUEUE_ITEM_SIZE (sizeof(system_event_t))
#define SYSTEM_QUEUE_LENGTH (10U)
#define SYSTEM_QUEUE_STORAGE_SIZE (SYSTEM_QUEUE_ITEM_SIZE * SYSTEM_QUEUE_LENGTH)

static void system_task_func(void* ctx)
{
    system_task_ctx_t* task_ctx = (system_task_ctx_t*)ctx;

    system_manager_t manager;
    TERMO_LOG_ON_ERR(pcTaskGetName(NULL),
                     system_manager_initialize(&manager, &task_ctx->config));

    while (1) {
        TERMO_LOG_ON_ERR(pcTaskGetName(NULL), system_manager_process(&manager));
        TERMO_DELAY(10);
    }
}

TaskHandle_t system_task_create_system_task(system_task_ctx_t const* task_ctx)
{
    static StaticTask_t system_task_buffer;
    static StackType_t system_task_stack[SYSTEM_TASK_STACK_DEPTH];

    return xTaskCreateStatic(system_task_func,
                             SYSTEM_TASK_NAME,
                             SYSTEM_TASK_STACK_DEPTH,
                             task_ctx,
                             SYSTEM_TASK_PRIORITY,
                             system_task_stack,
                             &system_task_buffer);
}

QueueHandle_t system_task_create_system_queue()
{
    static StaticQueue_t system_queue_buffer;
    static uint8_t system_queue_storage[SYSTEM_QUEUE_STORAGE_SIZE];

    return xQueueCreateStatic(SYSTEM_QUEUE_LENGTH,
                              SYSTEM_QUEUE_ITEM_SIZE,
                              system_queue_storage,
                              &system_queue_buffer);
}

termo_err_t system_task_initialize(system_task_ctx_t const* task_ctx)
{
    TERMO_ASSERT(task_ctx != NULL);

    TaskHandle_t system_task = system_task_create_system_task(task_ctx);
    if (system_task == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_task_manager_set(TERMO_TASK_TYPE_SYSTEM, system_task);

    QueueHandle_t system_queue = system_task_create_system_queue();
    if (system_queue == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_queue_manager_set(TERMO_QUEUE_TYPE_SYSTEM, system_queue);

    return TERMO_ERR_OK;
}