#ifndef CRAWLER_ITERATOR_H_
#define CRAWLER_ITERATOR_H_

#include <stddef.h>
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CRAWLER_STREAM_SUCCESS,
    CRAWLER_STREAM_ERROR,
    CRAWLER_STREAM_MISSING_ELEMENT
} CrawlerStreamResult;

typedef struct CrawlerInternalUTF8Stream {
    CrawlerBuffer* buffer;

    int current_code_point;
    size_t current_total_offset;

    size_t offset;
    size_t head;
} CrawlerUTF8Stream;

struct CrawlerInternalParserContext;

void crawler_stream_init(struct CrawlerInternalParserContext* parser);
CrawlerStreamResult crawler_stream_peek(struct CrawlerInternalParserContext* parser);
void crawler_stream_commit(struct CrawlerInternalParserContext* parser);
void crawler_stream_reset(struct CrawlerInternalParserContext* parser);
int crawler_stream_current(struct CrawlerInternalParserContext* parser);

#ifdef __cplusplus
}
#endif
#endif