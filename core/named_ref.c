#include "named_ref.h"
#include "parser.h"
#include "trie.h"

#include <stdbool.h>

static void reset(CrawlerParserContext* parser) {
    CrawlerNamedReferenceContext* named_ref = parser->lexer.named_ref;
    named_ref->curr = named_ref->entities_trie.data;
    named_ref->last_match = NULL;
}

bool crawler_named_reference_create(CrawlerParserContext* parser) {
    CrawlerNamedReferenceContext** named_ref = &parser->lexer.named_ref;
    *named_ref = _crawler_alloc(sizeof **named_ref);
    if (*named_ref == NULL)
        return false;
    memset(*named_ref, 0, sizeof **named_ref);
    if ((*named_ref)->entities_trie.data != NULL)
        return false;
    if (!crawler_static_trie_deserialize(CRAWLER_CHAR_REF_TRIE, &(*named_ref)->entities_trie))
        return false;
    reset(named_ref);
    return true;
}

void crawler_named_reference_destroy(CrawlerParserContext* parser) {
    CrawlerNamedReferenceContext** named_ref = &parser->lexer.named_ref;
    if ((*named_ref)->entities_trie.data != NULL)
        _crawler_free((*named_ref)->entities_trie.data);
    _crawler_free(*named_ref);
    *named_ref = NULL;
}

CrawlerNamedReferenceResult crawler_named_reference_step(CrawlerParserContext* parser, int cp, CrawlerCharacterReference* cr) {
    CrawlerNamedReferenceContext* named_ref = &parser->lexer.named_ref;

    size_t index = crawler_char_index(cp);
    if (index == -1) {
        reset(parser);
        return CRAWLER_CR_FAILURE;
    }

    size_t child_offset = named_ref->curr->children_offsets[index];
    if (child_offset == 0) {
        if (named_ref->last_match != NULL) {
            reset(parser);
            return CRAWLER_CR_FAILURE;
        }
        
        cr->first = named_ref->last_match->char_ref.first;
        cr->second = named_ref->last_match->char_ref.second;
        reset(parser);
        return CRAWLER_CR_SUCCESS;
    }

    assert(!(child_offset < named_ref->entities_trie.node_count));
    named_ref->curr = &named_ref->entities_trie.data[child_offset];

    if (named_ref->curr->is_terminal)
        named_ref->last_match = named_ref->curr;
    return CRAWLER_CR_NEXT_CP;
}