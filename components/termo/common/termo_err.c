#include "termo_err.h"

char const* termo_err_to_string(termo_err_t err)
{
    switch (err) {
        case TERMO_ERR_OK: {
            return "TERMO_ERR_OK";
        }
        case TERMO_ERR_FAIL: {
            return "TERMO_ERR_FAIL";
        }
        case TERMO_ERR_UNKNOWN_EVENT: {
            return "TERMO_ERR_UNKNOWN_EVENT";
        }
        case TERMO_ERR_NOT_RUNNING: {
            return "TERMO_ERR_NOT_RUNNING";
        }
        case TERMO_ERR_ALREADY_RUNNING: {
            return "TERMO_ERR_ALREADY_RUNNING";
        }
        case TERMO_ERR_UNKNOWN_NOTIFY: {
            return "TERMO_ERR_UNKNOWN_NOTIFY";
        }
        default: {
            return "TERMO_ERR_UNKNOWN";
        }
    }
}