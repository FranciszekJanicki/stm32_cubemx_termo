#include "termo_log.h"
#include <stdarg.h>
#include <stdio.h>

void termo_log(char const* format, ...)
{
    va_list list;
    va_start(list, ...);
    vprintf(format, list);
    va_end(list);
}