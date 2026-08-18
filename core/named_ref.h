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

    int last_character_matched;
} CrawlerNamedReferenceContext;

bool crawler_named_reference_create(CrawlerLexerContext* lexer);
void crawler_named_reference_destroy(CrawlerLexerContext* lexer);

CrawlerNamedReferenceResult crawler_named_reference_step(CrawlerParserContext* parser, int cp, CrawlerCharacterReference* cr);
int crawler_named_reference_get_last_matched_char(CrawlerParserContext* parser);

#ifdef __cplusplus
}
#endif
#endif