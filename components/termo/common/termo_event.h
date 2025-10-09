#ifndef COMMON_TERMO_EVENT_H
#define COMMON_TERMO_EVENT_H

typedef enum {
    CONTROL_EVENT_TYPE_START,
    CONTROL_EVENT_TYPE_STOP,
    CONTROL_EVENT_TYPE_REFERENCE,
} control_event_type_t;

typedef struct {
} control_event_payload_start_t;
typedef struct {
} control_event_payload_stop_t;
typedef struct {
    float temperature;
    float sampling_period;
} control_event_payload_reference_t;

typedef union {
    control_event_payload_start_t start;
    control_event_payload_stop_t stop;
    control_event_payload_reference_t reference;
} control_event_payload_t;

typedef struct {
    control_event_type_t type;
    control_event_payload_t payload;
} control_event_t;

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

#endif // COMMON_TERMO_EVENT_H