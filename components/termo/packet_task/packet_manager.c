#include "packet_manager.h"
#include "packet_in.h"
#include "packet_out.h"
#include "termo_common.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"
#include <string.h>

static char const* const TAG = "packet_manager";

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

    if (!packet_manager_send_system_event(
            &(system_event_t){.type = SYSTEM_EVENT_TYPE_STARTED,
                              .origin = SYSTEM_EVENT_ORIGIN_PACKET})) {
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

    if (!packet_manager_send_system_event(
            &(system_event_t){.type = SYSTEM_EVENT_TYPE_STOPPED,
                              .origin = SYSTEM_EVENT_ORIGIN_PACKET})) {
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

    packet_out_t packet = {
        .type = PACKET_OUT_TYPE_MEASURE,
        .payload.measure = {.temperature = measure->temperature,
                            .humidity = measure->humidity,
                            .pressure = measure->pressure}};

    uint8_t buffer[PACKET_OUT_SIZE] = {0};
    packet_out_encode(&packet, &buffer);

    CDC_Transmit_FS(buffer, PACKET_OUT_SIZE);

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

    return TERMO_ERR_OK;
}

termo_err_t packet_manager_initialize(packet_manager_t* manager,
                                      packet_config_t const* config)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(config != NULL);

    manager->is_running = false;
    manager->config = *config;

    if (!packet_manager_send_system_event(
            &(system_event_t){.type = SYSTEM_EVENT_TYPE_READY,
                              .origin = SYSTEM_EVENT_ORIGIN_PACKET})) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}
