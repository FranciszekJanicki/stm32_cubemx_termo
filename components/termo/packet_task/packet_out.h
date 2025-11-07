#ifndef COMMON_PACKET_OUT_H
#define COMMON_PACKET_OUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PACKET_OUT_TYPE_MEASURE,
} packet_out_type_t;

typedef struct {
    float temperature;
    float pressure;
    float humidity;
} packet_out_payload_measure_t;

typedef union {
    packet_out_payload_measure_t measure;
} packet_out_payload_t;

typedef struct {
    packet_out_type_t type;
    packet_out_payload_t payload;
} packet_out_t;

#ifdef USE_BINARY_PACKETS

#define PACKET_OUT_TYPE_OFFSET (0UL)
#define PACKET_OUT_TYPE_SIZE (sizeof(packet_out_type_t))

#define PACKET_OUT_PAYLOAD_OFFSET \
    (PACKET_OUT_TYPE_OFFSET + PACKET_OUT_TYPE_SIZE)
#define PACKET_OUT_PAYLOAD_SIZE (sizeof(packet_out_payload_t))

#define PACKET_OUT_SIZE (PACKET_OUT_TYPE_SIZE + PACKET_OUT_PAYLOAD_SIZE)

bool packet_out_encode(packet_out_t const* packet,
                       uint8_t (*buffer)[PACKET_OUT_SIZE]);

bool packet_out_decode(const uint8_t (*buffer)[PACKET_OUT_SIZE],
                       packet_out_t* packet);

#else

bool packet_out_encode(packet_out_t const* packet,
                       char* buffer,
                       size_t buffer_len);

bool packet_out_decode(char const* buffer,
                       size_t buffer_len,
                       packet_out_t* packet);

#endif

#endif // COMMON_PACKET_OUT_H