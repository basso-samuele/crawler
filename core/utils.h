#ifndef CRAWLER_UTILS_H_
#define CRAWLER_UTILS_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void* _crawler_alloc(size_t size);
void _crawler_free(void* ptr);

void crawler_debug(const char* fmt, ...);
bool crawler_is_big_endian();

char* crawler_read_file(const char* path);

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_UTILS_H_