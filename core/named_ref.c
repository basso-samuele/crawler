#include "named_ref.h"
#include "parser.h"
#include "trie.h"
#include "utils.h"

#include <stdbool.h>
#include <string.h>
#include <assert.h>

static void reset(CrawlerLexerContext* lexer) {
    CrawlerNamedReferenceContext* named_ref =
        (CrawlerNamedReferenceContext*)(lexer->named_ref);
    named_ref->curr = named_ref->entities_trie.data;
    named_ref->last_match = NULL;
    named_ref->last_character_matched = 0;
}

bool crawler_named_reference_create(CrawlerLexerContext* lexer) {
    CrawlerNamedReferenceContext** named_ref =
        (CrawlerNamedReferenceContext**)(&lexer->named_ref);
    *named_ref = _crawler_alloc(sizeof **named_ref);
    if (*named_ref == NULL)
        return false;
    memset(*named_ref, 0, sizeof **named_ref);
    if ((*named_ref)->entities_trie.data != NULL)
        return false;
    if (!crawler_static_trie_deserialize(CRAWLER_CHAR_REF_TRIE, &(*named_ref)->entities_trie))
        return false;
    reset(lexer);
    return true;
}

void crawler_named_reference_destroy(CrawlerLexerContext* lexer) {
    CrawlerNamedReferenceContext** named_ref =
        (CrawlerNamedReferenceContext**)(&lexer->named_ref);
    if ((*named_ref)->entities_trie.data != NULL)
        _crawler_free((*named_ref)->entities_trie.data);
    _crawler_free(*named_ref);
    *named_ref = NULL;
}

static CrawlerNamedReferenceResult final_step(CrawlerParserContext* parser, CrawlerCharacterReference* cr) {
    CrawlerNamedReferenceContext* named_ref =
        (CrawlerNamedReferenceContext*)(parser->lexer.named_ref);

    if (named_ref->last_match == NULL) {
        reset(&parser->lexer);
        return CRAWLER_CR_FAILURE;
    }

    cr->first = named_ref->last_match->char_ref.first;
    cr->second = named_ref->last_match->char_ref.second;
    reset(&parser->lexer);
    return CRAWLER_CR_SUCCESS;
}

CrawlerNamedReferenceResult crawler_named_reference_step(CrawlerParserContext* parser, int cp, CrawlerCharacterReference* cr) {
    CrawlerNamedReferenceContext* named_ref =
        (CrawlerNamedReferenceContext*)(parser->lexer.named_ref);

    size_t index = crawler_char_index(cp);
    if (index == -1)
        return final_step(parser, cr);

    size_t child_offset = named_ref->curr->children_offsets[index];
    if (child_offset == 0)
        return final_step(parser, cr);

    assert(child_offset < named_ref->entities_trie.node_count);
    named_ref->curr = &named_ref->entities_trie.data[child_offset];
    named_ref->last_character_matched = cp;

    if (named_ref->curr->is_terminal) {
        named_ref->last_match = named_ref->curr;
        crawler_stream_commit(&parser->is);
    }
    return CRAWLER_CR_NEXT_CP;
}

int crawler_named_reference_get_last_matched_char(CrawlerParserContext* parser) {
    CrawlerNamedReferenceContext* named_ref =
        (CrawlerNamedReferenceContext*)(parser->lexer.named_ref);

    return named_ref->last_character_matched;
}