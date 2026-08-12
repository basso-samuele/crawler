#include "memory.h"

#include <stdlib.h>

void* _crawler_alloc(size_t size) {
    return malloc(size);
}

void _crawler_free(void* ptr) {
    free(ptr);
}