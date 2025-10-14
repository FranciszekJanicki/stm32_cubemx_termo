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

static inline bool system_manager_send_termo_event(termo_event_t const* event)
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

static inline bool system_manager_send_packet_event(packet_event_t const* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueSend(termo_queue_manager_get(TERMO_QUEUE_TYPE_PACKET),
                      event,
                      pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool system_manager_has_system_event(void)
{
    return uxQueueMessagesWaiting(
               termo_queue_manager_get(TERMO_QUEUE_TYPE_SYSTEM)) > 0UL;
}

static inline bool system_manager_receive_system_event(system_event_t* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueReceive(termo_queue_manager_get(TERMO_QUEUE_TYPE_SYSTEM),
                         event,
                         pdMS_TO_TICKS(10)) == pdPASS;
}

static termo_err_t system_manager_notify_handler(system_manager_t* manager,
                                                 system_notify_t notify)
{
    TERMO_ASSERT(manager != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_termo_ready_handler(
    system_manager_t* manager,
    system_event_payload_termo_ready_t const* termo_ready)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(termo_ready != NULL);

    termo_event_t event = {.type = TERMO_EVENT_TYPE_START, .payload.start = {}};
    if (!system_manager_send_termo_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_termo_started_handler(
    system_manager_t* manager,
    system_event_payload_termo_started_t const* termo_started)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(termo_started != NULL);

    manager->is_termo_running = true;

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_termo_stopped_handler(
    system_manager_t* manager,
    system_event_payload_termo_stopped_t const* termo_stopped)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(termo_stopped != NULL);

    manager->is_termo_running = false;

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_termo_reference_handler(
    system_manager_t* manager,
    system_event_payload_termo_reference_t const* termo_reference)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(termo_reference != NULL);

    if (termo_reference->temperature == manager->reference_temperature &&
        termo_reference->sampling_time == manager->sampling_time) {
        return TERMO_ERR_OK;
    }

    if (manager->is_termo_running) {
        termo_event_t event = {
            .type = TERMO_EVENT_TYPE_REFERENCE,
            .payload.reference = {.temperature = termo_reference->temperature,
                                  .sampling_period =
                                      termo_reference->sampling_time}};
        if (!system_manager_send_termo_event(&event)) {
            return TERMO_ERR_FAIL;
        }
    }

    if (manager->is_display_running) {
        display_event_t event = {
            .type = DISPLAY_EVENT_TYPE_REFERENCE,
            .payload.reference = {.temperature = termo_reference->temperature,
                                  .sampling_time = termo_reference->sampling_time}};
        if (!system_manager_send_display_event(&event)) {
            return TERMO_ERR_FAIL;
        }
    }

    manager->reference_temperature = termo_reference->temperature;
    manager->sampling_time = termo_reference->sampling_time;

    TERMO_LOG(TAG,
              "New reference: temperature = %.2f, sampling_time = %.2f",
              termo_reference->temperature,
              termo_reference->sampling_time);

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_termo_measure_handler(
    system_manager_t* manager,
    system_event_payload_termo_measure_t const* termo_measure)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(termo_measure != NULL);

    if (manager->measure_humidity == termo_measure->humidity &&
        manager->measure_pressure == termo_measure->pressure &&
        manager->measure_temperature == termo_measure->temperature) {
        return TERMO_ERR_OK;
    }

    if (manager->is_display_running) {
        display_event_t event = {
            .type = DISPLAY_EVENT_TYPE_MEASURE,
            .payload.measure = {.humidity = termo_measure->humidity,
                                .pressure = termo_measure->pressure,
                                .temperature = termo_measure->temperature}};
        if (!system_manager_send_display_event(&event)) {
            return TERMO_ERR_FAIL;
        }
    }

    if (manager->is_packet_running) {
        packet_event_t event = {
            .type = PACKET_EVENT_TYPE_MEASURE,
            .payload.measure = {.humidity = termo_measure->humidity,
                                .pressure = termo_measure->pressure,
                                .temperature = termo_measure->temperature}};
        if (!system_manager_send_packet_event(&event)) {
            return TERMO_ERR_FAIL;
        }
    }

    manager->measure_humidity = termo_measure->humidity;
    manager->measure_pressure = termo_measure->pressure;
    manager->measure_temperature = termo_measure->temperature;

    TERMO_LOG(TAG,
              "New measure: temperature = %.2f, humidity = %.2f, pressure = "
              "%.2f",
              termo_measure->temperature,
              termo_measure->humidity,
              termo_measure->pressure);

    return TERMO_ERR_OK;
}

static termo_err_t system_manager_event_handler(system_manager_t* manager,
                                                system_event_t const* event)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(event != NULL);

    switch (event->type) {
        case SYSTEM_EVENT_TYPE_TERMO_READY: {
            return system_manager_event_termo_ready_handler(
                manager,
                &event->payload.termo_ready);
        }
        case SYSTEM_EVENT_TYPE_TERMO_STARTED: {
            return system_manager_event_termo_started_handler(
                manager,
                &event->payload.termo_started);
        }
        case SYSTEM_EVENT_TYPE_TERMO_STOPPED: {
            return system_manager_event_termo_stopped_handler(
                manager,
                &event->payload.termo_stopped);
        }
        case SYSTEM_EVENT_TYPE_TERMO_REFERENCE: {
            return system_manager_termo_reference_handler(
                manager,
                &event->payload.termo_reference);
        }
        case SYSTEM_EVENT_TYPE_TERMO_MEASURE: {
            return system_manager_termo_measure_handler(
                manager,
                &event->payload.termo_measure);
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

    manager->config = *config;
    manager->is_termo_running = false;
    manager->is_display_running = false;
    manager->is_packet_running = false;
    manager->reference_temperature = 0.0F;
    manager->measure_temperature = 0.0F;
    manager->measure_humidity = 0.0F;
    manager->measure_pressure = 0.0F;
    manager->sampling_time = 0.0F;

    return TERMO_ERR_OK;
}
