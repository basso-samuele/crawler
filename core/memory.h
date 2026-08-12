#include <stddef.h>

#ifndef CRAWLER_MEMORY_H_
#define CRAWLER_MEMORY_H_

void* _crawler_alloc(size_t size);
void _crawler_free(void* ptr);

#endif