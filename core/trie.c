#include "trie.h"
#include <string.h>
#include <stddef.h>

void crawler_trie_node_init(CrawlerTrieNode* node) {
    memset(node, 0, sizeof(CrawlerTrieNode));
}

static size_t char_index(unsigned char c) {
    if (c >= 'a' && c <= 'z') {
        return (size_t)(c - 'a');
    }

    if (c >= 'A' && c <= 'Z') {
        return (size_t)(c - 'A');
    }

    if (c >= '0' && c <= '9') {
        return (size_t)(26 + (c - '0'));
    }

    return -1;
}

bool crawler_trie_insert(CrawlerTrieNode** root, const char* literal, size_t length, int first, int second) {
    if (root == NULL || literal == NULL) return false;

    if (*root == NULL) {
        *root = malloc(sizeof **root);
        if (*root == NULL) return false;
        crawler_trie_node_init(*root);
    }

    CrawlerTrieNode* curr = *root;
    for (size_t depth = 0; depth < length; depth++) {
        size_t index = char_index(literal[depth]);
        if (index == -1) return false;
        CrawlerTrieNode** child = &curr->children[index];
        if (*child == NULL) {
            *child = malloc(sizeof **child);
            if (*child == NULL) return false;
            crawler_trie_node_init(*child);
        }
        curr = *child;
    }

    if (curr->is_terminal) return false;

    curr->char_ref.first = first;
    curr->char_ref.second = second;
    curr->is_terminal = true;

    return true;
}

bool crawler_trie_query(const CrawlerTrieNode* root, const char* literal, size_t length, CrawlerCharacterReference* output) {
    if (root == NULL || literal == NULL || output == NULL) return false;
    if (!(length > 0)) return false;

    const CrawlerTrieNode* curr = root;
    for (size_t depth = 0; depth < length; depth++) {
        size_t index = char_index(literal[depth]);
        if (index == -1) return false;
        const CrawlerTrieNode* child = curr->children[index];
        if (child == NULL) return false;
        curr = child;
    }

    if (curr->is_terminal) {
        *output = curr->char_ref;
        return true;
    }

    return false;
}