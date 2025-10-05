#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H

#include "FreeRTOS.h"
#include "termo_log.h"
#include "task.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG

#define TERMO_DELAY(MS) vTaskDelay(pdMS_TO_TICKS(MS))

#define TERMO_PANIC()             \
    do {                          \
        taskDISABLE_INTERRUPTS(); \
        while (1)                 \
            ;                     \
    } while (0)

#define TERMO_RET_ON_ERR(ERR)      \
    do {                           \
        termo_err_t err = (ERR);   \
        if (err != TERMO_ERR_OK) { \
            return err;            \
        }                          \
    } while (0)

#ifdef DEBUG

#define TERMO_LOG(TAG, FMT, ...) \
    termo_log("[%s] " FMT "\n\r", TAG, ##__VA_ARGS__)

#define TERMO_LOG_FUNC(TAG) TERMO_LOG(TAG, "%s", __func__)

#define TERMO_LOG_ON_ERR(TAG, ERR)                          \
    do {                                                    \
        termo_err_t err = (ERR);                            \
        if (err != TERMO_ERR_OK) {                          \
            TERMO_LOG(TAG, "%s", termo_err_to_string(err)); \
        }                                                   \
    } while (0)

#define TERMO_ASSERT(EXPR)                            \
    do {                                              \
        if (!(EXPR)) {                                \
            TERMO_LOG("TERMO_ASSERT",                 \
                      "%s: %d: Assertion failed: %s", \
                      __FILE__,                       \
                      __LINE__,                       \
                      #EXPR);                         \
            vTaskDelay(100);                          \
            TERMO_PANIC();                            \
        }                                             \
    } while (0)

#define TERMO_ERR_CHECK(ERR) TERMO_ASSERT((ERR) == TERMO_ERR_OK)

#else

#define TERMO_LOG(TAG, FMT, ...) \
    do {                         \
    } while (0)

#define TERMO_LOG_FUNC(TAG) \
    do {                    \
    } while (0)

#define TERMO_LOG_ON_ERR(TAG, ERR) \
    do {                           \
    } while (0)

#define TERMO_ASSERT(EXPR) \
    do {                   \
        (void)(EXPR);      \
    } while (0)

#define TERMO_ERR_CHECK(ERR) \
    do {                     \
    } while (0)

#endif // DEBUG

#endif // COMMON_UTILITY_H
