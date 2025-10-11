
#include "display_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"

#define DISPLAY_TASK_STACK_DEPTH (5000UL / sizeof(StackType_t))
#define DISPLAY_TASK_NAME ("display_task")
#define DISPLAY_TASK_PRIORITY (1UL)

#define DISPLAY_QUEUE_ITEM_SIZE (sizeof(display_event_t))
#define DISPLAY_QUEUE_LENGTH (10U)
#define DISPLAY_QUEUE_STORAGE_SIZE \
    (DISPLAY_QUEUE_ITEM_SIZE * DISPLAY_QUEUE_LENGTH)

static void display_task_func(void* ctx)
{
    display_task_ctx_t* task_ctx = (display_task_ctx_t*)ctx;

    display_manager_t manager;
    TERMO_LOG_ON_ERR(pcTaskGetName(NULL),
                     display_manager_initialize(&manager, &task_ctx->config));

    while (1) {
        TERMO_LOG_ON_ERR(pcTaskGetName(NULL),
                         display_manager_process(&manager));
        TERMO_DELAY(10);
    }
}

TaskHandle_t display_task_create_display_task(
    display_task_ctx_t const* task_ctx)
{
    static StaticTask_t display_task_buffer;
    static StackType_t display_task_stack[DISPLAY_TASK_STACK_DEPTH];

    return xTaskCreateStatic(display_task_func,
                             DISPLAY_TASK_NAME,
                             DISPLAY_TASK_STACK_DEPTH,
                             task_ctx,
                             DISPLAY_TASK_PRIORITY,
                             display_task_stack,
                             &display_task_buffer);
}

QueueHandle_t display_task_create_display_queue()
{
    static StaticQueue_t display_queue_buffer;
    static uint8_t display_queue_storage[DISPLAY_QUEUE_STORAGE_SIZE];

    return xQueueCreateStatic(DISPLAY_QUEUE_LENGTH,
                              DISPLAY_QUEUE_ITEM_SIZE,
                              display_queue_storage,
                              &display_queue_buffer);
}

termo_err_t display_task_initialize(display_task_ctx_t const* task_ctx)
{
    TERMO_ASSERT(task_ctx != NULL);

    TaskHandle_t display_task = display_task_create_display_task(task_ctx);
    if (display_task == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_task_manager_set(TERMO_TASK_TYPE_DISPLAY, display_task);

    QueueHandle_t display_queue = display_task_create_display_queue();
    if (display_queue == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_queue_manager_set(TERMO_QUEUE_TYPE_DISPLAY, display_queue);

    return TERMO_ERR_OK;
}