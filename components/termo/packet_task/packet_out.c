#include "packet_out.h"
#include "termo_common.h"
#include <string.h>

#ifdef USE_BINARY_PACKETS

static inline void packet_out_type_encode(packet_out_type_t type,
                                          uint8_t* buffer)
{
    buffer[0] = (type >> 24U) & 0xFFU;
    buffer[1] = (type >> 16U) & 0xFFU;
    buffer[2] = (type >> 8U) & 0xFFU;
    buffer[3] = type & 0xFFU;
}

static inline void packet_out_payload_measure_encode(
    packet_out_payload_measure_t const* measure,
    uint8_t* buffer)
{
    uint32_t temperature;
    memcpy(&temperature, &measure->temperature, sizeof(temperature));
    buffer[0] = (temperature >> 24U) & 0xFFU;
    buffer[1] = (temperature >> 16U) & 0xFFU;
    buffer[2] = (temperature >> 8U) & 0xFFU;
    buffer[3] = temperature & 0xFFU;

    uint32_t pressure;
    memcpy(&pressure, &measure->pressure, sizeof(pressure));
    buffer[4] = (pressure >> 24U) & 0xFFU;
    buffer[5] = (pressure >> 16U) & 0xFFU;
    buffer[6] = (pressure >> 8U) & 0xFFU;
    buffer[7] = pressure & 0xFFU;

    uint32_t humidity;
    memcpy(&humidity, &measure->humidity, sizeof(humidity));
    buffer[8] = (humidity >> 24U) & 0xFFU;
    buffer[9] = (humidity >> 16U) & 0xFFU;
    buffer[10] = (humidity >> 8U) & 0xFFU;
    buffer[11] = humidity & 0xFFU;
}

static inline void packet_out_payload_encode(
    packet_out_type_t type,
    packet_out_payload_t const* payload,
    uint8_t* buffer)
{
    switch (type) {
        case PACKET_OUT_TYPE_MEASURE: {
            packet_out_payload_measure_encode(&payload->measure, buffer);
            break;
        }
        default: {
            break;
        }
    }
}

bool packet_out_encode(packet_out_t const* packet,
                       uint8_t (*buffer)[PACKET_OUT_SIZE])
{
    if (packet == NULL || buffer == NULL) {
        return false;
    }

    uint8_t* type_buffer = *buffer + PACKET_OUT_TYPE_OFFSET;
    packet_out_type_encode(packet->type, type_buffer);

    uint8_t* payload_buffer = *buffer + PACKET_OUT_PAYLOAD_OFFSET;
    packet_out_payload_encode(packet->type, &packet->payload, payload_buffer);

    return true;
}

static inline void packet_out_type_decode(uint8_t const* buffer,
                                          packet_out_type_t* type)
{
    *type = ((buffer[0] & 0xFFU) << 24U) | ((buffer[1] & 0xFFU) << 16U) |
            ((buffer[2] & 0xFFU) << 8U) | (buffer[3] & 0xFFU);
}

static inline void packet_out_payload_measure_decode(
    uint8_t const* buffer,
    packet_out_payload_measure_t* measure)
{
    uint32_t temperature = ((buffer[0] & 0xFFU) << 24U) |
                           ((buffer[1] & 0xFFU) << 16U) |
                           ((buffer[2] & 0xFFU) << 8U) | (buffer[3] & 0xFFU);
    memcpy(&measure->temperature, &temperature, sizeof(temperature));

    uint32_t pressure = ((buffer[4] & 0xFFU) << 24U) |
                        ((buffer[5] & 0xFFU) << 16U) |
                        ((buffer[6] & 0xFFU) << 8U) | (buffer[7] & 0xFFU);
    memcpy(&measure->pressure, &pressure, sizeof(pressure));

    uint32_t humidity = ((buffer[8] & 0xFFU) << 24U) |
                        ((buffer[9] & 0xFFU) << 16U) |
                        ((buffer[10] & 0xFFU) << 8U) | (buffer[11] & 0xFFU);
    memcpy(&measure->humidity, &humidity, sizeof(humidity));
}

static inline void packet_out_payload_decode(uint8_t const* buffer,
                                             packet_out_type_t type,
                                             packet_out_payload_t* payload)
{
    switch (type) {
        case PACKET_OUT_TYPE_MEASURE: {
            packet_out_payload_measure_decode(buffer, &payload->measure);
            break;
        }
        default: {
            break;
        }
    }
}

bool packet_out_decode(const uint8_t (*buffer)[PACKET_OUT_SIZE],
                       packet_out_t* packet)
{
    if (buffer == NULL || packet == NULL) {
        return false;
    }

    uint8_t const* type_buffer = *buffer + PACKET_OUT_TYPE_OFFSET;
    packet_out_type_decode(type_buffer, &packet->type);

    uint8_t const* payload_buffer = *buffer + PACKET_OUT_PAYLOAD_OFFSET;
    packet_out_payload_decode(payload_buffer, packet->type, &packet->payload);

    return true;
}

#else

bool packet_out_encode(packet_out_t const* packet,
                       char* buffer,
                       size_t buffer_len)
{
    if (packet == NULL || buffer == NULL || buffer_len == 0UL) {
        return false;
    }

    int written_len = 0;
    if (packet->type == PACKET_OUT_TYPE_MEASURE) {
        written_len = snprintf(buffer,
                               buffer_len,
                               "{\"packet_type\":%d,"
                               "\"packet_payload\":{"
                               "\"temperature\":%f,"
                               "\"pressure\":%f,"
                               "\"humidity\":%f}}\n",
                               packet->type,
                               packet->payload.measure.temperature,
                               packet->payload.measure.pressure,
                               packet->payload.measure.humidity);
    }
    if (written_len < 0) {
        return false;
    }

    buffer[written_len] = '\0';
    return true;
}

bool packet_out_decode(char const* buffer,
                       size_t buffer_len,
                       packet_out_t* packet)
{
    if (buffer == NULL || buffer_len == 0UL || packet == NULL) {
        return false;
    }

    if (strlen(buffer) != buffer_len) {
        return false;
    }

    int type;
    int scanned_num = sscanf(buffer, "{\"packet_type\":%d", &type);
    if (scanned_num != 1) {
        return false;
    }
    packet->type = (packet_out_type_t)type;

    if (packet->type == PACKET_OUT_TYPE_MEASURE) {
        float temperature = 0.0F;
        float pressure = 0.0F;
        float humidity = 0.0F;
        scanned_num = sscanf(buffer,
                             "{\"packet_type\":%d,"
                             "\"packet_payload\":{\"temperature\":%f,"
                             "\"pressure\":%f,"
                             "\"humidity\":%f}}\n",
                             &type,
                             &temperature,
                             &pressure,
                             &humidity);
        if (scanned_num != 4) {
            return false;
        }
        packet->payload.measure.temperature = temperature;
        packet->payload.measure.pressure = pressure;
        packet->payload.measure.humidity = humidity;
    }

    return true;
}

#endif
