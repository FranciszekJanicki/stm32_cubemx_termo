#include "control_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"

#define CONTROL_TASK_STACK_DEPTH (5000UL / sizeof(StackType_t))
#define CONTROL_TASK_NAME ("control_task")
#define CONTROL_TASK_PRIORITY (1UL)

#define CONTROL_QUEUE_ITEM_SIZE (sizeof(control_event_t))
#define CONTROL_QUEUE_LENGTH (10U)
#define CONTROL_QUEUE_STORAGE_SIZE \
    (CONTROL_QUEUE_ITEM_SIZE * CONTROL_QUEUE_LENGTH)

static void control_task_func(void* ctx)
{
    control_task_ctx_t* task_ctx = (control_task_ctx_t*)ctx;

    control_manager_t manager;
    TERMO_LOG_ON_ERR(pcTaskGetName(NULL),
                     control_manager_initialize(&manager,
                                                &task_ctx->config,
                                                &task_ctx->params));

    while (1) {
        TERMO_LOG_ON_ERR(pcTaskGetName(NULL),
                         control_manager_process(&manager));
        TERMO_DELAY(10);
    }
}

TaskHandle_t control_task_create_control_task(
    control_task_ctx_t const* task_ctx)
{
    static StaticTask_t control_task_buffer;
    static StackType_t control_task_stack[CONTROL_TASK_STACK_DEPTH];

    return xTaskCreateStatic(control_task_func,
                             CONTROL_TASK_NAME,
                             CONTROL_TASK_STACK_DEPTH,
                             task_ctx,
                             CONTROL_TASK_PRIORITY,
                             control_task_stack,
                             &control_task_buffer);
}

QueueHandle_t control_task_create_control_queue()
{
    static StaticQueue_t control_queue_buffer;
    static uint8_t control_queue_storage[CONTROL_QUEUE_STORAGE_SIZE];

    return xQueueCreateStatic(CONTROL_QUEUE_LENGTH,
                              CONTROL_QUEUE_ITEM_SIZE,
                              control_queue_storage,
                              &control_queue_buffer);
}

termo_err_t control_task_initialize(control_task_ctx_t const* task_ctx)
{
    TERMO_ASSERT(task_ctx != NULL);

    TaskHandle_t control_task = control_task_create_control_task(task_ctx);
    if (control_task == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_task_manager_set(TERMO_TASK_TYPE_CONTROL, control_task);

    QueueHandle_t control_queue = control_task_create_control_queue();
    if (control_queue == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_queue_manager_set(TERMO_QUEUE_TYPE_CONTROL, control_queue);

    return TERMO_ERR_OK;
}

void control_task_delta_timer_callback(void)
{
    BaseType_t task_woken = pdFALSE;
    xTaskNotifyFromISR(termo_task_manager_get(TERMO_TASK_TYPE_CONTROL),
                       CONTROL_NOTIFY_DELTA_TIMER,
                       eSetBits,
                       &task_woken);
    portYIELD_FROM_ISR(task_woken);
}

#undef CONTROL_TASK_STACK_DEPTH
#undef CONTROL_TASK_NAME
#undef CONTROL_TASK_PRIORITY

#undef CONTROL_QUEUE_ITEM_SIZE
#undef CONTROL_QUEUE_LENGTH
#undef CONTROL_QUEUE_STORAGE_SIZE