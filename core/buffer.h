#ifndef CRAWLER_BUFFER_H_
#define CRAWLER_BUFFER_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned char* base;
    size_t size;
    bool eof;
} CrawlerBuffer;

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_BUFFER_H_