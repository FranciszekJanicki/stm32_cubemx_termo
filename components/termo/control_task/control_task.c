#include "control_task.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "termo_common.h"

#define CONTROL_TASK_STACK_DEPTH (5000UL / sizeof(StackType_t))
#define CONTROL_TASK_NAME ("control_task")
#define CONTROL_TASK_PRIORITY (1UL)
#define CONTROL_TASK_CORE_AFFINITY (0U)

static void control_task_func(void* ctx)
{
    control_task_ctx_t* task_ctx = (control_task_ctx_t*)ctx;

    control_manager_t manager;
    TERMO_ERR_CHECK(control_manager_initialize(&manager, &task_ctx->config));

    while (1) {
        TERMO_ERR_CHECK(control_manager_process(&manager));
        TERMO_DELAY(10);
    }
}

TaskHandle_t control_task_create_task(control_task_ctx_t const* task_ctx)
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

termo_err_t control_task_initialize(control_task_ctx_t const* task_ctx)
{
    TERMO_ASSERT(task_ctx != NULL);

    TaskHandle_t control_task = control_task_create_task(task_ctx);
    if (control_task == NULL) {
        return TERMO_ERR_FAIL;
    }

    termo_task_manager_set(TERMO_TASK_TYPE_CONTROL, control_task);

    return TERMO_ERR_OK;
}