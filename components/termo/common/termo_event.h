#ifndef COMMON_TERMO_EVENT_H
#define COMMON_TERMO_EVENT_H

typedef enum {
    SYSTEM_EVENT_ORIGIN_CONTROL,
    SYSTEM_EVENT_ORIGIN_DISPLAY,
    SYSTEM_EVENT_ORIGIN_PACKET,
} system_event_origin_t;

typedef enum {
    SYSTEM_EVENT_TYPE_TERMO_READY,
    SYSTEM_EVENT_TYPE_TERMO_STARTED,
    SYSTEM_EVENT_TYPE_TERMO_STOPPED,
    SYSTEM_EVENT_TYPE_TERMO_REFERENCE,
    SYSTEM_EVENT_TYPE_TERMO_MEASURE,
    SYSTEM_EVENT_TYPE_PACKET_READY,
    SYSTEM_EVENT_TYPE_PACKET_STARTED,
    SYSTEM_EVENT_TYPE_PACKET_STOPPED,
    SYSTEM_EVENT_TYPE_DISPLAY_READY,
    SYSTEM_EVENT_TYPE_DISPLAY_STARTED,
    SYSTEM_EVENT_TYPE_DISPLAY_STOPPED,
} system_event_type_t;

typedef struct {
} system_event_payload_termo_ready_t;

typedef struct {
} system_event_payload_termo_started_t;

typedef struct {
} system_event_payload_termo_stopped_t;

typedef struct {
    float temperature;
    float sampling_time;
} system_event_payload_termo_reference_t;

typedef struct {
    float temperature;
    float humidity;
    float pressure;
} system_event_payload_termo_measure_t;

typedef struct {
} system_event_payload_packet_ready_t;

typedef struct {
} system_event_payload_packet_started_t;

typedef struct {
} system_event_payload_packet_stopped_t;

typedef struct {
} system_event_payload_display_ready_t;

typedef struct {
} system_event_payload_display_started_t;

typedef struct {
} system_event_payload_display_stopped_t;

typedef union {
    system_event_payload_termo_ready_t termo_ready;
    system_event_payload_termo_started_t termo_started;
    system_event_payload_termo_stopped_t termo_stopped;
    system_event_payload_termo_measure_t termo_measure;
    system_event_payload_termo_reference_t termo_reference;
    system_event_payload_packet_ready_t packet_ready;
    system_event_payload_packet_started_t packet_started;
    system_event_payload_packet_stopped_t packet_stopped;
    system_event_payload_display_ready_t display_ready;
    system_event_payload_display_started_t display_started;
    system_event_payload_display_stopped_t display_stopped;
} system_event_payload_t;

typedef struct {
    system_event_type_t type;
    system_event_origin_t origin;
    system_event_payload_t payload;
} system_event_t;

typedef enum {
    TERMO_EVENT_TYPE_START,
    TERMO_EVENT_TYPE_STOP,
    TERMO_EVENT_TYPE_REFERENCE,
} termo_event_type_t;

typedef struct {
} termo_event_payload_start_t;

typedef struct {
} termo_event_payload_stop_t;

typedef struct {
    float temperature;
    float sampling_period;
} termo_event_payload_reference_t;

typedef union {
    termo_event_payload_start_t start;
    termo_event_payload_stop_t stop;
    termo_event_payload_reference_t reference;
} termo_event_payload_t;

typedef struct {
    termo_event_type_t type;
    termo_event_payload_t payload;
} termo_event_t;

typedef enum {
    DISPLAY_EVENT_TYPE_START,
    DISPLAY_EVENT_TYPE_STOP,
    DISPLAY_EVENT_TYPE_MEASURE,
    DISPLAY_EVENT_TYPE_REFERENCE,
} display_event_type_t;

typedef struct {
} display_event_payload_start_t;

typedef struct {
} display_event_payload_stop_t;

typedef struct {
    float temperature;
    float humidity;
    float pressure;
} display_event_payload_measure_t;

typedef struct {
    float sampling_time;
    float temperature;
} display_event_payload_reference_t;

typedef union {
    display_event_payload_start_t start;
    display_event_payload_stop_t stop;
    display_event_payload_measure_t measure;
    display_event_payload_reference_t reference;
} display_event_payload_t;

typedef struct {
    display_event_type_t type;
    display_event_payload_t payload;
} display_event_t;

typedef enum {
    PACKET_EVENT_TYPE_START,
    PACKET_EVENT_TYPE_STOP,
    PACKET_EVENT_TYPE_MEASURE,
} packet_event_type_t;

typedef struct {
} packet_event_payload_start_t;

typedef struct {
} packet_event_payload_stop_t;

typedef struct {
    float temperature;
    float humidity;
    float pressure;
} packet_event_payload_measure_t;

typedef union {
    packet_event_payload_start_t start;
    packet_event_payload_stop_t stop;
    packet_event_payload_measure_t measure;
} packet_event_payload_t;

typedef struct {
    packet_event_type_t type;
    packet_event_payload_t payload;
} packet_event_t;

#endif // COMMON_TERMO_EVENT_H