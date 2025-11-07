
#include "packet_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"

#define PACKET_TASK_STACK_DEPTH (10000UL / sizeof(StackType_t))
#define PACKET_TASK_NAME ("packet_task")
#define PACKET_TASK_PRIORITY (1UL)

#define PACKET_QUEUE_ITEM_SIZE (sizeof(packet_event_t))
#define PACKET_QUEUE_LENGTH (10U)
#define PACKET_QUEUE_STORAGE_SIZE (PACKET_QUEUE_ITEM_SIZE * PACKET_QUEUE_LENGTH)

static void packet_task_func(void* ctx)
{
    packet_task_ctx_t* task_ctx = (packet_task_ctx_t*)ctx;

    packet_manager_t manager;
    TERMO_LOG_ON_ERR(pcTaskGetName(NULL),
                     packet_manager_initialize(&manager, &task_ctx->config));

    while (1) {
        TERMO_LOG_ON_ERR(pcTaskGetName(NULL), packet_manager_process(&manager));
        TERMO_DELAY(10);
    }
}

TaskHandle_t packet_task_create_packet_task(packet_task_ctx_t const* task_ctx)
{
    static StaticTask_t packet_task_buffer;
    static StackType_t packet_task_stack[PACKET_TASK_STACK_DEPTH];

    return xTaskCreateStatic(packet_task_func,
                             PACKET_TASK_NAME,
                             PACKET_TASK_STACK_DEPTH,
                             task_ctx,
                             PACKET_TASK_PRIORITY,
                             packet_task_stack,
                             &packet_task_buffer);
}

QueueHandle_t packet_task_create_packet_queue()
{
    static StaticQueue_t packet_queue_buffer;
    static uint8_t packet_queue_storage[PACKET_QUEUE_STORAGE_SIZE];

    return xQueueCreateStatic(PACKET_QUEUE_LENGTH,
                              PACKET_QUEUE_ITEM_SIZE,
                              packet_queue_storage,
                              &packet_queue_buffer);
}

termo_err_t packet_task_initialize(packet_task_ctx_t const* task_ctx)
{
    TERMO_ASSERT(task_ctx != NULL);

    TaskHandle_t packet_task = packet_task_create_packet_task(task_ctx);
    if (packet_task == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_task_manager_set(TERMO_TASK_TYPE_PACKET, packet_task);

    QueueHandle_t packet_queue = packet_task_create_packet_queue();
    if (packet_queue == NULL) {
        return TERMO_ERR_FAIL;
    }
    termo_queue_manager_set(TERMO_QUEUE_TYPE_PACKET, packet_queue);

    return TERMO_ERR_OK;
}

void packet_task_rx_complete_callback(void)
{
    BaseType_t task_woken = pdFALSE;
    xTaskNotifyFromISR(termo_task_manager_get(TERMO_TASK_TYPE_PACKET),
                       PACKET_NOTIFY_RX_COMPLETE,
                       eSetBits,
                       &task_woken);
    portYIELD_FROM_ISR(task_woken);
}
