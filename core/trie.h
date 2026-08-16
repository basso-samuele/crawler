#ifndef CRAWLER_TRIE_H_
#define CRAWLER_TRIE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// A character reference may have one or two codepoints
typedef struct {
    int first;
    int second;
} CrawlerCharacterReference;

// Lowercase letters.
#define CRAWLER_KEY_SPACE_SIZE 36

typedef struct CrawlerInternalTrieNode {
    CrawlerCharacterReference char_ref;
    struct CrawlerInternalTrieNode* children[CRAWLER_KEY_SPACE_SIZE];
    bool is_terminal;

    /* Used during (de)serialization. */
    uint16_t node_offset;
} CrawlerTrieNode;

void crawler_trie_node_init(CrawlerTrieNode* node);

bool crawler_trie_insert(CrawlerTrieNode** root, const char* literal, size_t length, int first, int second);
bool crawler_trie_query(const CrawlerTrieNode* root, const char* literal, size_t length, CrawlerCharacterReference* output);

typedef struct CrawlerInternalStaticTrieNode {
    CrawlerCharacterReference char_ref;
    uint16_t children_offsets[CRAWLER_KEY_SPACE_SIZE];
    bool is_terminal;
} CrawlerStaticTrieNode;

#ifdef __cpluplus
}
#endif
#endif