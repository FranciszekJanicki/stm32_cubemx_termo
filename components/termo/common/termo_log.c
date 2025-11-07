#include "termo_log.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define STATIC_BUFFER_LEN (1000U)

extern int _write(int, char*, int);

void termo_log(char const* format, ...)
{
    va_list list;
    va_start(list, ...);
    int needed_len = vsnprintf(NULL, 0UL, format, list) + 1;

    char* buffer;
    size_t buffer_len;
    bool used_heap_buffer;
    if (needed_len <= STATIC_BUFFER_LEN) {
        static char static_buffer[STATIC_BUFFER_LEN];
        buffer = static_buffer;
        buffer_len = sizeof(static_buffer);
        used_heap_buffer = false;
    } else {
        char* heap_buffer = malloc(needed_len);
        if (heap_buffer == NULL)
            return;
        buffer = heap_buffer;
        buffer_len = needed_len;
        used_heap_buffer = true;
    }

    int written_len = vsnprintf(buffer, buffer_len, format, list);
    va_end(list);

    if (written_len < 0) {
        if (used_heap_buffer) {
            free(buffer);
        }
    }

    _write(1, buffer, written_len);

    if (used_heap_buffer) {
        free(buffer);
    }
}

#undef STATIC_BUFFER_LEN