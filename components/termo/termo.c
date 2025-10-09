#include "termo.h"
#include "FreeRTOS.h"
#include "task.h"

void termo_initialize(termo_config_t const* config)
{
    TERMO_ASSERT(config != NULL);

    TERMO_ERR_CHECK(control_task_initialize(&config->control_ctx));
    TERMO_ERR_CHECK(display_task_initialize(&config->display_ctx));

    vTaskStartScheduler();
}