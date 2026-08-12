#ifndef CRAWLER_TOKEN_H_
#define CRAWLER_TOKEN_H_

#include "tags.h"
#include "string_buffer.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CRAWLER_TOKEN_DOCTYPE,
    CRAWLER_TOKEN_START_TAG,
    CRAWLER_TOKEN_END_TAG,
    CRAWLER_TOKEN_COMMENT,
    CRAWLER_TOKEN_CHARACTER,
    CRAWLER_TOKEN_EOF,
    CRAWLER_TOKEN_TAG,
    CRAWLER_TOKEN_PROCESSING_INSTRUCTION,
    CRAWLER_TOKEN_TYPE_UNKNOWN
} CrawlerTokenType;

typedef struct {
    CrawlerString name;
    bool has_public_identifier;
    CrawlerString public_identifier;
    bool has_system_identifier;
    CrawlerString system_identifier;
    bool force_quirks;
} CrawlerTokenDocType;

typedef struct {
    CrawlerString name;
    bool is_self_closing;
    // attributes
} CrawlerStartTag;

typedef struct CrawlerInternalToken {
    CrawlerTokenType type;
    union {
        CrawlerTokenDocType doc_type;
        CrawlerStartTag start_tag;
        CrawlerString end_tag;
        CrawlerString str;
    } data;
} CrawlerToken;

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_TOKEN_H_