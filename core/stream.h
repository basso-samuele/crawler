#ifndef CRAWLER_ITERATOR_H_
#define CRAWLER_ITERATOR_H_

#include <stddef.h>
#include <stdbool.h>

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

    size_t current_code_point_offset;
    size_t offset;
    size_t head;

    int current_code_point;
    bool reconsume;
} CrawlerUTF8Stream;

struct CrawlerInternalParserContext;
CrawlerStreamResult crawler_stream_peek(struct CrawlerInternalParserContext* parser, int* cp);
CrawlerStreamResult crawler_stream_get(struct CrawlerInternalParserContext* parser);
void crawler_stream_reconsume(struct CrawlerInternalParserContext* parser);

void crawler_stream_init(CrawlerUTF8Stream* stream);
void crawler_stream_commit(CrawlerUTF8Stream* stream);
void crawler_stream_reset(CrawlerUTF8Stream* stream);

bool crawler_stream_consume_match(CrawlerUTF8Stream* stream, const char* prefix, size_t length, bool case_sensitive);

#ifdef __cplusplus
}
#endif
#endif