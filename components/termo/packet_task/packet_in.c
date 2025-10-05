#include "packet_in.h"
#include "termo_common.h"
#include <string.h>

static inline void packet_in_type_encode(packet_in_type_t type, uint8_t* buffer)
{
    buffer[0] = (type >> 24U) & 0xFFU;
    buffer[1] = (type >> 16U) & 0xFFU;
    buffer[2] = (type >> 8U) & 0xFFU;
    buffer[3] = type & 0xFFU;
}

static inline void packet_in_payload_reference_encode(
    packet_in_payload_reference_t const* reference,
    uint8_t* buffer)
{
    uint32_t temperature;
    memcpy(&temperature, &reference->temperature, sizeof(temperature));
    buffer[0] = (temperature >> 24U) & 0xFFU;
    buffer[1] = (temperature >> 16U) & 0xFFU;
    buffer[2] = (temperature >> 8U) & 0xFFU;
    buffer[3] = temperature & 0xFFU;

    uint32_t sampling_time;
    memcpy(&sampling_time, &reference->sampling_time, sizeof(sampling_time));
    buffer[4] = (sampling_time >> 24U) & 0xFFU;
    buffer[5] = (sampling_time >> 16U) & 0xFFU;
    buffer[6] = (sampling_time >> 8U) & 0xFFU;
    buffer[7] = sampling_time & 0xFFU;
}

static inline void packet_in_payload_encode(packet_in_type_t type,
                                            packet_in_payload_t const* payload,
                                            uint8_t* buffer)
{
    switch (type) {
        case PACKET_IN_TYPE_REFERENCE: {
            packet_in_payload_reference_encode(&payload->reference, buffer);
            break;
        }
        default: {
            break;
        }
    }
}

void packet_in_encode(packet_in_t const* packet,
                      uint8_t (*buffer)[PACKET_IN_SIZE])
{
    TERMO_ASSERT(packet != NULL);
    TERMO_ASSERT(buffer != NULL);

    uint8_t* type_buffer = *buffer + PACKET_IN_TYPE_OFFSET;
    packet_in_type_encode(packet->type, type_buffer);

    uint8_t* payload_buffer = type_buffer + PACKET_IN_PAYLOAD_OFFSET;
    packet_in_payload_encode(packet->type, &packet->payload, payload_buffer);
}

static inline void packet_in_type_decode(uint8_t const* buffer,
                                         packet_in_type_t* type)
{
    *type = ((buffer[0] & 0xFFU) << 24U) | ((buffer[1] & 0xFFU) << 16U) |
            ((buffer[2] & 0XFFU) << 8U) | (buffer[3] & 0xFFU);
}

static inline void packet_in_payload_reference_decode(
    uint8_t const* buffer,
    packet_in_payload_reference_t* reference)
{
    uint32_t temperature = ((buffer[0] & 0xFFU) << 24U) |
                           ((buffer[1] & 0xFFU) << 16U) |
                           ((buffer[2] & 0xFFU) << 8U) | (buffer[3] & 0xFFU);
    memcpy(&reference->temperature,
           &temperature,
           sizeof(reference->temperature));

    uint32_t sampling_time = ((buffer[4] & 0xFFU) << 24U) |
                             ((buffer[5] & 0xFFU) << 16U) |
                             ((buffer[6] & 0xFFU) << 8U) | (buffer[7] & 0xFFU);
    memcpy(&reference->sampling_time,
           &sampling_time,
           sizeof(reference->sampling_time));
}

static inline void packet_in_payload_decode(uint8_t const* buffer,
                                            packet_in_type_t type,
                                            packet_in_payload_t* payload)
{
    switch (type) {
        case PACKET_IN_TYPE_REFERENCE: {
            packet_in_payload_reference_decode(buffer, &payload->reference);
            break;
        }
        default: {
            break;
        }
    }
}

void packet_in_decode(const uint8_t (*buffer)[PACKET_IN_SIZE],
                      packet_in_t* packet)
{
    uint8_t const* type_buffer = *buffer + PACKET_IN_TYPE_OFFSET;
    packet_in_type_decode(type_buffer, &packet->type);

    uint8_t const* payload_buffer = type_buffer + PACKET_IN_PAYLOAD_OFFSET;
    packet_in_payload_decode(payload_buffer, packet->type, &packet->payload);
}
