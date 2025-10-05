#include "termo.h"
#include "FreeRTOS.h"
#include "task.h"

void termo_initialize(termo_config_t const* config)
{
    TERMO_ERR_CHECK(control_task_initialize(&config->control_ctx));

    vTaskStartScheduler();
}