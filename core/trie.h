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

#define CRAWLER_KEY_SPACE_SIZE 60

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

void crawler_trie_destroy(CrawlerTrieNode** root);

bool crawler_trie_bft_offset(CrawlerTrieNode* root, uint16_t* count);
bool crawler_trie_bft_serialize(const char* path, CrawlerTrieNode* root);

typedef struct {
    CrawlerCharacterReference char_ref;
    uint16_t children_offsets[CRAWLER_KEY_SPACE_SIZE];
    bool is_terminal;
} CrawlerStaticTrieNode;

typedef struct {
    CrawlerStaticTrieNode* data;
    uint16_t node_count;
} CrawlerStaticTrie;

size_t crawler_char_index(unsigned char c);

bool crawler_static_trie_deserialize(const char* path, CrawlerStaticTrie* t);
bool crawler_static_trie_query(const CrawlerStaticTrie* t, const char* literal, size_t length, CrawlerCharacterReference* output);

#ifdef __cplusplus
}
#endif
#endif