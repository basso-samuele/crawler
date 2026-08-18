#ifndef CRAWLER_CHAR_REF_H_
#define CRAWLER_CHAR_REF_H_

#include "trie.h"
#include "parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CRAWLER_CR_SUCCESS,
    CRAWLER_CR_FAILURE,
    CRAWLER_CR_NEXT_CP
} CrawlerNamedReferenceResult;

typedef struct {
    CrawlerStaticTrie entities_trie;
    CrawlerStaticTrieNode* last_match;
    CrawlerStaticTrieNode* curr;
} CrawlerNamedReferenceContext;

bool crawler_named_reference_create(CrawlerParserContext* parser);
void crawler_named_reference_destroy(CrawlerParserContext* parser);

CrawlerNamedReferenceResult crawler_named_reference_step(CrawlerParserContext* parser, int cp, CrawlerCharacterReference* cr);

#ifdef __cplusplus
}
#endif
#endif