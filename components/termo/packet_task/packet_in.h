#ifndef COMMON_PACKET_IN_H
#define COMMON_PACKET_IN_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PACKET_IN_TYPE_REFERENCE,
} packet_in_type_t;

typedef struct {
    float temperature;
    float sampling_time;
} packet_in_payload_reference_t;

typedef union {
    packet_in_payload_reference_t reference;
} packet_in_payload_t;

typedef struct {
    packet_in_type_t type;
    packet_in_payload_t payload;
} packet_in_t;

#ifdef USE_BINARY_PACKETS

#define PACKET_IN_TYPE_OFFSET (0UL)
#define PACKET_IN_TYPE_SIZE (sizeof(packet_in_type_t))

#define PACKET_IN_PAYLOAD_OFFSET (PACKET_IN_TYPE_OFFSET + PACKET_IN_TYPE_SIZE)
#define PACKET_IN_PAYLOAD_SIZE (sizeof(packet_in_payload_t))

#define PACKET_IN_SIZE (PACKET_IN_TYPE_SIZE + PACKET_IN_PAYLOAD_SIZE)

bool packet_in_encode(packet_in_t const* packet,
                      uint8_t (*buffer)[PACKET_IN_SIZE]);

bool packet_in_decode(const uint8_t (*buffer)[PACKET_IN_SIZE],
                      packet_in_t* packet);

#else

bool packet_in_encode(packet_in_t const* packet,
                      char* buffer,
                      size_t buffer_len);

bool packet_in_decode(char const* buffer,
                      size_t buffer_len,
                      packet_in_t* packet);

#endif

#endif // COMMON_PACKET_IN_H