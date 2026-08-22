#ifndef CRAWLER_TOKEN_H_
#define CRAWLER_TOKEN_H_

#include "attributes.h"
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
    CRAWLER_TOKEN_PROCESSING_INSTRUCTION,
    CRAWLER_TOKEN_TYPE_UNKNOWN
} CrawlerTokenType;

typedef struct {
    CrawlerString name;
    CrawlerString public_identifier;
    CrawlerString system_identifier;
    bool has_public_identifier;
    bool has_system_identifier;
    bool force_quirks;
} CrawlerTokenDocType;

typedef struct {
    CrawlerString name;
    bool is_self_closing;
    CrawlerAttributeNode* attributes;
} CrawlerStartTag;

typedef struct {
    CrawlerString data;
    CrawlerString target;
} CrawlerProcessingInstruction;

typedef struct CrawlerInternalToken {
    CrawlerTokenType type;
    union {
        CrawlerProcessingInstruction proc_in;
        CrawlerTokenDocType doc_type;
        CrawlerStartTag start_tag;
        CrawlerString end_tag;
        CrawlerString str;
    } data;
} CrawlerToken;

// Initializes token to CRAWLER_TOKEN_TYPE_UNKNOWN. This state
// represents the fact that no resources are being held by the structure
void crawler_token_init(CrawlerToken* token);
void crawler_token_destroy(CrawlerToken* token);
void crawler_token_clone(CrawlerToken* destination, CrawlerToken* source);

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_TOKEN_H_