
#include "display_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"

#define DISPLAY_TASK_STACK_DEPTH (5000UL / sizeof(StackType_t))
#define DISPLAY_TASK_NAME ("display_task")
#define DISPLAY_TASK_PRIORITY (1UL)
#define DISPLAY_TASK_CORE_AFFINITY (0U)

static void display_task_func(void* ctx)
{
    display_task_ctx_t* task_ctx = (display_task_ctx_t*)ctx;

    display_manager_t manager;
    TERMO_ERR_CHECK(display_manager_initialize(&manager, &task_ctx->config));

    while (1) {
        TERMO_ERR_CHECK(display_manager_process(&manager));
        TERMO_DELAY(10);
    }
}

TaskHandle_t display_task_create_task(display_task_ctx_t const* task_ctx)
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

termo_err_t display_task_initialize(display_task_ctx_t const* task_ctx)
{
    TERMO_ASSERT(task_ctx != NULL);

    TaskHandle_t display_task = display_task_create_task(task_ctx);
    if (display_task == NULL) {
        return TERMO_ERR_FAIL;
    }

    termo_task_manager_set(TERMO_TASK_TYPE_DISPLAY, display_task);

    return TERMO_ERR_OK;
}