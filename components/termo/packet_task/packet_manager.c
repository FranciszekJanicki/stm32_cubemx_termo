#include "packet_manager.h"
#include "packet_in.h"
#include "packet_out.h"
#include "termo_common.h"
#include <string.h>

static char const* const TAG = "packet_manager";

static inline bool packet_manager_transmit_packet_out(
    packet_manager_t* manager,
    packet_out_t const* packet)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(packet != NULL);

    char buffer[100];
    memset(buffer, 0, sizeof(buffer));

    if (!packet_out_encode(packet, buffer, sizeof(buffer))) {
        return false;
    }

    return HAL_UART_Transmit(manager->config.packet_uart_bus,
                             buffer,
                             strlen(buffer),
                             100) == HAL_OK;
}

static inline bool packet_manager_receive_packet_in(packet_manager_t* manager,
                                                    packet_in_t* packet)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(packet != NULL);

    char buffer[1000];
    memset(buffer, 0, sizeof(buffer));

#ifdef PACKET_IN_TEST
    snprintf(buffer,
             sizeof(buffer),
             "{\"packet_type\":%d,"
             "\"packet_payload\":{\"temperature\":%f,"
             "\"sampling_time\":%f}}\n",
             0,
             25.0F,
             0.001F);

    TERMO_LOG(TAG, "packet_in_test: %s", buffer);
#else
    if (HAL_UART_Receive(manager->config.packet_uart_bus,
                         buffer,
                         sizeof(buffer),
                         100) != HAL_OK) {
        return false;
    }
#endif

    return packet_in_decode(buffer, strlen(buffer), packet);
}

static inline bool packet_manager_send_system_event(system_event_t const* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueSend(termo_queue_manager_get(TERMO_QUEUE_TYPE_SYSTEM),
                      event,
                      pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool packet_manager_receive_packet_notify(packet_notify_t* notify)
{
    TERMO_ASSERT(notify != NULL);

    return xTaskNotifyWait(0x00,
                           PACKET_NOTIFY_ALL,
                           (uint32_t*)notify,
                           pdMS_TO_TICKS(10)) == pdPASS;
}

static inline bool packet_manager_has_packet_event(void)
{
    return uxQueueMessagesWaiting(
               termo_queue_manager_get(TERMO_QUEUE_TYPE_PACKET)) > 0UL;
}

static inline bool packet_manager_receive_packet_event(packet_event_t* event)
{
    TERMO_ASSERT(event != NULL);

    return xQueueReceive(termo_queue_manager_get(TERMO_QUEUE_TYPE_PACKET),
                         event,
                         pdMS_TO_TICKS(10)) == pdPASS;
}

static termo_err_t packet_manager_notify_handler(packet_manager_t* manager,
                                                 packet_notify_t notify)
{
    TERMO_ASSERT(manager != NULL);

    return TERMO_ERR_OK;
}

static termo_err_t packet_manager_event_start_handler(
    packet_manager_t* manager,
    packet_event_payload_start_t const* start)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(start != NULL);

    if (manager->is_running) {
        return TERMO_ERR_ALREADY_RUNNING;
    }

    system_event_t event = {.origin = SYSTEM_EVENT_ORIGIN_PACKET,
                            .type = SYSTEM_EVENT_TYPE_PACKET_STARTED,
                            .payload.packet_started = {}};
    if (!packet_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = true;

    return TERMO_ERR_OK;
}

static termo_err_t packet_manager_event_stop_handler(
    packet_manager_t* manager,
    packet_event_payload_stop_t const* stop)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(stop != NULL);

    if (!manager->is_running) {
        return TERMO_ERR_NOT_RUNNING;
    }

    system_event_t event = {.origin = SYSTEM_EVENT_ORIGIN_PACKET,
                            .type = SYSTEM_EVENT_TYPE_PACKET_STOPPED,
                            .payload.packet_stopped = {}};
    if (!packet_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    manager->is_running = false;

    return TERMO_ERR_OK;
}

static termo_err_t packet_manager_event_measure_handler(
    packet_manager_t* manager,
    packet_event_payload_measure_t const* measure)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(measure != NULL);

    if (!manager->is_running) {
        return TERMO_ERR_NOT_RUNNING;
    }

    packet_out_t packet = {
        .type = PACKET_OUT_TYPE_MEASURE,
        .payload.measure = {.temperature = measure->temperature,
                            .humidity = measure->humidity,
                            .pressure = measure->pressure}};

    if (!packet_manager_transmit_packet_out(manager, &packet)) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}

static termo_err_t packet_manager_event_handler(packet_manager_t* manager,
                                                packet_event_t const* event)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(event != NULL);

    switch (event->type) {
        case PACKET_EVENT_TYPE_START: {
            return packet_manager_event_start_handler(manager,
                                                      &event->payload.start);
        }
        case PACKET_EVENT_TYPE_STOP: {
            return packet_manager_event_stop_handler(manager,
                                                     &event->payload.stop);
        }
        case PACKET_EVENT_TYPE_MEASURE: {
            return packet_manager_event_measure_handler(
                manager,
                &event->payload.measure);
        }
        default: {
            return TERMO_ERR_UNKNOWN_EVENT;
        }
    }
}

static termo_err_t packet_manager_packet_in_reference_handler(
    packet_manager_t* manager,
    packet_in_payload_reference_t const* reference)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(reference != NULL);

    if (!manager->is_running) {
        return TERMO_ERR_NOT_RUNNING;
    }

    system_event_t event = {
        .origin = SYSTEM_EVENT_ORIGIN_PACKET,
        .type = SYSTEM_EVENT_TYPE_TERMO_REFERENCE,
        .payload.termo_reference = {.temperature = reference->temperature,
                                    .sampling_time = reference->sampling_time}};
    if (!packet_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}

static termo_err_t packet_manager_packet_in_handler(packet_manager_t* manager,
                                                    packet_in_t const* packet)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(packet != NULL);

    switch (packet->type) {
        case PACKET_IN_TYPE_REFERENCE: {
            return packet_manager_packet_in_reference_handler(
                manager,
                &packet->payload.reference);
        }
        default: {
            return TERMO_ERR_UNKNOWN_EVENT;
        }
    }
}

termo_err_t packet_manager_process(packet_manager_t* manager)
{
    TERMO_ASSERT(manager != NULL);

    packet_notify_t notify;
    if (packet_manager_receive_packet_notify(&notify)) {
        TERMO_RET_ON_ERR(packet_manager_notify_handler(manager, notify));
    }

    packet_event_t event;
    while (packet_manager_has_packet_event()) {
        if (packet_manager_receive_packet_event(&event)) {
            TERMO_RET_ON_ERR(packet_manager_event_handler(manager, &event));
        }
    }

    packet_in_t packet;
    if (packet_manager_receive_packet_in(manager, &packet)) {
        packet_manager_packet_in_handler(manager, &packet);
    }

    return TERMO_ERR_OK;
}

termo_err_t packet_manager_initialize(packet_manager_t* manager,
                                      packet_config_t const* config)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(config != NULL);

    manager->is_running = false;
    manager->config = *config;

    system_event_t event = {.origin = SYSTEM_EVENT_ORIGIN_PACKET,
                            .type = SYSTEM_EVENT_TYPE_PACKET_READY,
                            .payload.packet_ready = {}};
    if (!packet_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}
