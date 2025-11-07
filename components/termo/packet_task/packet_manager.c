#include "packet_manager.h"
#include "packet_in.h"
#include "packet_out.h"
#include "termo_common.h"
#include <string.h>

static char const* const TAG = "packet_manager";

static inline bool packet_manager_prepare_packet_out(packet_manager_t* manager,
                                                     packet_out_t const* packet)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(packet != NULL);

    bool result = packet_out_encode(packet,
                                    (char*)manager->transmit_buffer,
                                    sizeof(manager->transmit_buffer));

    if (result) {
        manager->is_transmit_pending = true;
    }

    return result;
}

static inline bool packet_manager_transmit_packet_out(
    packet_manager_t* manager,
    packet_out_t const* packet)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(packet != NULL);

    if (!packet_manager_prepare_packet_out(manager, packet)) {
        return false;
    }

    HAL_StatusTypeDef err =
        HAL_UART_Transmit(manager->config.packet_uart_bus,
                          manager->transmit_buffer,
                          strlen((char*)manager->transmit_buffer),
                          HAL_MAX_DELAY);

    memset(manager->transmit_buffer, 0, sizeof(manager->transmit_buffer));

    return err == HAL_OK;
}

static inline bool packet_manager_parse_packet_in(packet_manager_t* manager,
                                                  packet_in_t* packet)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(packet != NULL);

    bool result = packet_in_decode((char*)manager->receive_buffer,
                                   strlen((char*)manager->receive_buffer),
                                   packet);

    if (result) {
        manager->is_receive_pending = true;
    }

    return result;
}

static inline bool packet_manager_receive_packet_in(packet_manager_t* manager,
                                                    packet_in_t* packet)
{
    TERMO_ASSERT(manager != NULL);
    TERMO_ASSERT(packet != NULL);

    memset(manager->receive_buffer, 0, sizeof(manager->receive_buffer));

    uint32_t start_tick = HAL_GetTick();
    uint32_t total_timeout_ms = 10000U;
    uint32_t inter_byte_timeout_ms = 100U;

    size_t index = 0UL;
    uint8_t byte = 0U;

    bool got_first_byte = false;
    uint32_t last_byte_tick = start_tick;

    while (1) {
        uint32_t now = HAL_GetTick();

        if (!got_first_byte && (now - start_tick >= total_timeout_ms)) {
            break;
        }

        if (got_first_byte && (now - last_byte_tick >= inter_byte_timeout_ms)) {
            break;
        }

        if (HAL_UART_Receive(manager->config.packet_uart_bus, &byte, 1, 100U) ==
            HAL_OK) {
            got_first_byte = true;
            last_byte_tick = HAL_GetTick();

            if (byte == '\n') {
                break;
            }

            if (index >= (sizeof(manager->receive_buffer) - 1U)) {
                break;
            }

            manager->receive_buffer[index++] = byte;
        }
    }

    manager->receive_buffer[index] = '\0';
    TERMO_LOG(TAG, "received: %s", (char*)manager->receive_buffer);

    return packet_manager_parse_packet_in(manager, packet);
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

static termo_err_t packet_manager_notify_rx_complete_handler(
    packet_manager_t* manager)
{
    TERMO_LOG_FUNC(TAG);
    TERMO_ASSERT(manager != NULL);

    // HAL_UART_Receive_IT(manager->config.packet_uart_bus,
    //                     manager->receive_buffer,
    //                     sizeof(manager->receive_buffer));

    return TERMO_ERR_OK;
}

static termo_err_t packet_manager_notify_handler(packet_manager_t* manager,
                                                 packet_notify_t notify)
{
    TERMO_ASSERT(manager != NULL);

    if ((notify & PACKET_NOTIFY_RX_COMPLETE) == PACKET_NOTIFY_RX_COMPLETE) {
    }

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
    manager->is_transmit_pending = false;
    manager->is_receive_pending = false;
    manager->config = *config;

    memset(manager->transmit_buffer, 0, sizeof(manager->transmit_buffer));
    memset(manager->receive_buffer, 0, sizeof(manager->receive_buffer));

    system_event_t event = {.origin = SYSTEM_EVENT_ORIGIN_PACKET,
                            .type = SYSTEM_EVENT_TYPE_PACKET_READY,
                            .payload.packet_ready = {}};
    if (!packet_manager_send_system_event(&event)) {
        return TERMO_ERR_FAIL;
    }

    return TERMO_ERR_OK;
}
