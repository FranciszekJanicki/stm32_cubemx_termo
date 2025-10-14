#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"
#include "termo_task.h"

#define TERMO_TASK_STACK_DEPTH (5000UL / sizeof(StackType_t))
#define TERMO_TASK_NAME ("termo_task")
#define TERMO_TASK_PRIORITY (1UL)

#define TERMO_QUEUE_ITEM_SIZE (sizeof(termo_event_t))
#define TERMO_QUEUE_LENGTH (10U)
#define TERMO_QUEUE_STORAGE_SIZE (TERMO_QUEUE_ITEM_SIZE * TERMO_QUEUE_LENGTH)

static void termo_task_func(void* ctx)
{
    termo_task_ctx_t* task_ctx = (termo_task_ctx_t*)ctx;

    termo_manager_t manager;
    TERMO_LOG_ON_ERR(pcTaskGetName(NULL),
                     termo_manager_initialize(&manager,
                                              &task_ctx->config,
                                              &task_ctx->params));

    while (1) {
        TERMO_LOG_ON_ERR(pcTaskGetName(NULL), termo_manager_process(&manager));
        TERMO_DELAY(10);
    }
}

TaskHandle_t termo_task_create_termo_task(termo_task_ctx_t const* task_ctx)
{
    static StaticTask_t termo_task_buffer;
    static StackType_t termo_task_stack[TERMO_TASK_STACK_DEPTH];

    return xTaskCreateStatic(termo_task_func,
                             TERMO_TASK_NAME,
                             TERMO_TASK_STACK_DEPTH,
                             task_ctx,
                             TERMO_TASK_PRIORITY,
                             termo_task_stack,
                             &termo_task_buffer);
}

QueueHandle_t termo_task_create_termo_queue()
{
    static StaticQueue_t termo_queue_buffer;
    static uint8_t termo_queue_storage[TERMO_QUEUE_STORAGE_SIZE];

    return xQueueCreateStatic(TERMO_QUEUE_LENGTH,
                              TERMO_QUEUE_ITEM_SIZE,
                              termo_queue_storage,
                              &termo_queue_buffer);
}

termo_err_t termo_task_initialize(termo_task_ctx_t const* task_ctx)
{
    TERMO_ASSERT(task_ctx != NULL);

    TaskHandle_t termo_task = termo_task_create_termo_task(task_ctx);
    if (termo_task == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_task_manager_set(TERMO_TASK_TYPE_CONTROL, termo_task);

    QueueHandle_t termo_queue = termo_task_create_termo_queue();
    if (termo_queue == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_queue_manager_set(TERMO_QUEUE_TYPE_CONTROL, termo_queue);

    return TERMO_ERR_OK;
}

void termo_task_delta_timer_callback(void)
{
    BaseType_t task_woken = pdFALSE;
    xTaskNotifyFromISR(termo_task_manager_get(TERMO_TASK_TYPE_CONTROL),
                       TERMO_NOTIFY_DELTA_TIMER,
                       eSetBits,
                       &task_woken);
    portYIELD_FROM_ISR(task_woken);
}

#undef TERMO_TASK_STACK_DEPTH
#undef TERMO_TASK_NAME
#undef TERMO_TASK_PRIORITY

#undef TERMO_QUEUE_ITEM_SIZE
#undef TERMO_QUEUE_LENGTH
#undef TERMO_QUEUE_STORAGE_SIZE