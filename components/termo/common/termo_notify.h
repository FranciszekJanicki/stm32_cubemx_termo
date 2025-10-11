#ifndef COMMON_TERMO_NOTIFY_H
#define COMMON_TERMO_NOTIFY_H

typedef enum {
    SYSTEM_NOTIFY_ALL = (1 << 0),
} system_notify_t;

typedef enum {
    CONTROL_NOTIFY_TEMP_READY = (1 << 0),
    CONTROL_NOTIFY_DELTA_TIMER = (1 << 1),
    CONTROL_NOTIFY_ALL =
        (CONTROL_NOTIFY_TEMP_READY | CONTROL_NOTIFY_DELTA_TIMER),
} control_notify_t;

typedef enum {
    DISPLAY_NOTIFY_ALL = (1 << 0),
} display_notify_t;

#endif // COMMON_TERMO_NOTIFY_H