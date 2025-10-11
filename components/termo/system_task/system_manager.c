#include "system_manager.h"
#include "termo_common.h"
#include <string.h>

static char const* const TAG = "system_manager";

static inline bool system_manager_receive_system_notify(system_notify_t* notify)
{
    TERMO_ASSERT(notify != NULL);

    return xTaskNotifyWait(0x00,
                           SYSTEM_NOTIFY_ALL,
                           (uint32_t*)notify,
                           pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool system_manager_send_control_event(
    control_event_t const* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueSend(termo_queue_manager_get(TERMO_QUEUE_TYPE_CONTROL),
                      event,
                      pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool system_manager_send_display_event(
    display_event_t const* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueSend(termo_queue_manager_get(TERMO_QUEUE_TYPE_DISPLAY),
                      event,
                      pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool system_manager_has_system_event(void)
{
    return uxQueueMessagesWaiting(
               termo_queue_manager_get(TERMO_QUEUE_TYPE_CONTROL)) > 0UL;
}

static inline bool system_manager_receive_system_event(system_event_t* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueReceive(termo_queue_manager_get(TERMO_QUEUE_TYPE_CONTROL),
                         event,
                         pdMS_TO_TICKS(10)) == pdPASS;
}

static termo_err_t system_manager_notify_handler(system_manager_t* manager,
                                                 system_notify_t notify)
{
    TERMO_ASSERT(manager != NULL);

    return TERMO_ERR_UNKNOWN_NOTIFY;
}

static termo_err_t system_manager_event_ready_handler(
    system_manager_t* manager,
    system_event_origin_t origin,
    system_event_payload_ready_t const* ready)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(ready != NULL);

    switch (origin) {
        case SYSTEM_EVENT_ORIGIN_CONTROL: {
            if (!system_manager_send_control_event(
                    &(control_event_t){.type = CONTROL_EVENT_TYPE_START})) {
                return TERMO_ERR_FAIL;
            }
            break;
        }
        case SYSTEM_EVENT_ORIGIN_DISPLAY: {
            if (!system_manager_send_display_event(
                    &(display_event_t){.type = DISPLAY_EVENT_TYPE_START})) {
                return TERMO_ERR_FAIL;
            }
            break;
        }
        default: {
            return TERMO_ERR_UNKNOWN_EVENT;
        }
    }

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_started_handler(
    system_manager_t* manager,
    system_event_origin_t origin,
    system_event_payload_started_t const* started)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(started != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_stopped_handler(
    system_manager_t* manager,
    system_event_origin_t origin,
    system_event_payload_stopped_t const* stopped)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(stopped != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_reference_handler(
    system_manager_t* manager,
    system_event_origin_t origin,
    system_event_payload_reference_t const* reference)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(reference != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_measure_handler(
    system_manager_t* manager,
    system_event_origin_t origin,
    system_event_payload_measure_t const* measure)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(measure != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_handler(system_manager_t* manager,
                                                system_event_t const* event)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(event != NULL);

    switch (event->type) {
        case SYSTEM_EVENT_TYPE_READY: {
            return system_manager_event_ready_handler(manager,
                                                      event->origin,
                                                      &event->payload.ready);
        }
        case SYSTEM_EVENT_TYPE_STARTED: {
            return system_manager_event_started_handler(
                manager,
                event->origin,
                &event->payload.started);
        }
        case SYSTEM_EVENT_TYPE_STOPPED: {
            return system_manager_event_stopped_handler(
                manager,
                event->origin,
                &event->payload.stopped);
        }
        case SYSTEM_EVENT_TYPE_REFERENCE: {
            return system_manager_event_reference_handler(
                manager,
                event->origin,
                &event->payload.reference);
        }
        case SYSTEM_EVENT_TYPE_MEASURE: {
            return system_manager_event_measure_handler(
                manager,
                event->origin,
                &event->payload.measure);
        }
        default: {
            return TERMO_ERR_UNKNOWN_EVENT;
        }
    }
}

termo_err_t system_manager_process(system_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    system_notify_t notify;
    if (system_manager_receive_system_notify(&notify)) {
        TERMO_RET_ON_ERR(system_manager_notify_handler(manager, notify));
    }

    system_event_t event;
    while (system_manager_has_system_event()) {
        if (system_manager_receive_system_event(&event)) {
            TERMO_RET_ON_ERR(system_manager_event_handler(manager, &event));
        }
    }

    return TERMO_ERR_OK;
}

termo_err_t system_manager_initialize(system_manager_t* manager,
                                      system_config_t const* config)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(config != NULL);

    memcpy(&manager->config, config, sizeof(manager->config));

    return TERMO_ERR_OK;
}
