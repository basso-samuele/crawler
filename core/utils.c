#include "utils.h"

#include <stdarg.h>
#include <stdio.h>

void crawler_debug(const char* fmt, ...) {
#ifdef CRAWLER_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
#endif
}