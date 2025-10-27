
#include "termo.h"
#include "FreeRTOS.h"
#include "task.h"

void termo_initialize(termo_ctx_t const* config)
{
    TERMO_ASSERT(config != NULL);

    TERMO_ERR_CHECK(system_task_initialize(&config->system_ctx));
    TERMO_ERR_CHECK(termo_task_initialize(&config->termo_ctx));
    TERMO_ERR_CHECK(display_task_initialize(&config->display_ctx));
    TERMO_ERR_CHECK(packet_task_initialize(&config->packet_ctx));

    vTaskStartScheduler();
}
