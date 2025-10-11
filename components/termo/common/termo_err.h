#ifndef COMMON_TERMO_ERR_H
#define COMMON_TERMO_ERR_H

typedef enum {
    TERMO_ERR_OK = 0,
    TERMO_ERR_FAIL,
    TERMO_ERR_UNKNOWN_EVENT,
    TERMO_ERR_NOT_RUNNING,
    TERMO_ERR_ALREADY_RUNNING,
    TERMO_ERR_UNKNOWN_NOTIFY,
} termo_err_t;

char const* termo_err_to_string(termo_err_t err);

#endif // COMMON_TERMO_ERR_H