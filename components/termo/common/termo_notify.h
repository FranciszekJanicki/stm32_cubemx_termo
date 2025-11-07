#ifndef COMMON_TERMO_NOTIFY_H
#define COMMON_TERMO_NOTIFY_H

typedef enum {
    SYSTEM_NOTIFY_ALL = (1 << 0),
} system_notify_t;

typedef enum {
    TERMO_NOTIFY_TEMP_READY = (1 << 0),
    TERMO_NOTIFY_DELTA_TIMER = (1 << 1),
    TERMO_NOTIFY_ALL = (TERMO_NOTIFY_TEMP_READY | TERMO_NOTIFY_DELTA_TIMER),
} termo_notify_t;

typedef enum {
    DISPLAY_NOTIFY_ALL = (1 << 0),
} display_notify_t;

typedef enum {
    PACKET_NOTIFY_RX_COMPLETE = (1 << 0),
    PACKET_NOTIFY_ALL = (PACKET_NOTIFY_RX_COMPLETE),
} packet_notify_t;

#endif // COMMON_TERMO_NOTIFY_H