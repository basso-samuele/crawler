#ifndef CRAWLER_MEMORY_H_
#define CRAWLER_MEMORY_H_

#include <stddef.h>

void* _crawler_alloc(size_t size);
void _crawler_free(void* ptr);

#endif