#include "utils.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

void crawler_debug(const char* fmt, ...) {
#ifdef CRAWLER_DEBUG
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    fflush(stdout);
#endif
}

bool crawler_is_big_endian() {
    uint16_t value = 1;
    uint8_t* bytes = (uint8_t*)&value;
    return bytes[0] == 0;
}

void* _crawler_alloc(size_t size) {
    return malloc(size);
}

void _crawler_free(void* ptr) {
    free(ptr);
}

char* crawler_read_file(const char* path) {
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    char* result = _crawler_alloc((size_t)size*sizeof *result + 1);
    if (result == NULL) {
        fclose(fp);
        return NULL;
    }

    size_t nread = fread(result, sizeof *result, size, fp);
    fclose(fp);
    if (nread != (size_t)size) {
        _crawler_free(result);
        return NULL;
    }

    result[size] = '\0';
    return result;
}