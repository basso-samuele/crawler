#include "trie.h"
#include "queue.h"
#include "utils.h"

#include <string.h>
#include <stddef.h>
#include <stdio.h>

void crawler_trie_node_init(CrawlerTrieNode* node) {
    if (node == NULL) return;
    memset(node, 0, sizeof(CrawlerTrieNode));
}

static const size_t kAsciiToTrieIndex[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0–15
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 16–31
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 32–47
    -1,  0,  1,  2,  3,  4,  5,  6,  7, -1, -1, 60, -1, -1, -1, -1,  // 48–63 (49–56 = '1'–'8')
    -1,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,  // 64–79 (65–90 = 'A'–'Z')
    23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, -1, -1, -1, -1, -1,  // 80–95
    -1, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,  // 96–111 (97–122 = 'a'–'z')
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, -1, -1, -1, -1, -1   // 112–127
};

size_t crawler_char_index(unsigned char c) {
    if (c > 127)
        return -1;
    return kAsciiToTrieIndex[c];
}

bool crawler_trie_insert(CrawlerTrieNode** root, const char* literal, size_t length, int first, int second) {
    if (root == NULL || literal == NULL) return false;

    if (*root == NULL) {
        *root = _crawler_alloc(sizeof **root);
        if (*root == NULL) return false;
        crawler_trie_node_init(*root);
    }

    CrawlerTrieNode* curr = *root;
    for (size_t depth = 0; depth < length; depth++) {
        size_t index = crawler_char_index(literal[depth]);
        if (index == -1) return false;
        CrawlerTrieNode** child = &curr->children[index];
        if (*child == NULL) {
            *child = _crawler_alloc(sizeof **child);
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
        size_t index = crawler_char_index(literal[depth]);
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

void crawler_trie_destroy(CrawlerTrieNode** root) {
    if (root == NULL) return;
    if (*root == NULL) return;

    CrawlerQueue queue;
    crawler_queue_init(&queue);
    crawler_queue_create(&queue, 8);

    CrawlerTrieNode* node = *root;
    do {
        for (size_t i = 0; i < CRAWLER_KEY_SPACE_SIZE; i++) {
            if (node->children[i] != NULL) {
                crawler_enqueue(&queue, node->children[i]);
            }
        }
        _crawler_free(node);
        node = crawler_dequeue(&queue);
    } while(node != NULL);

    crawler_queue_destroy(&queue);
    *root = NULL;
}

bool crawler_trie_bft_offset(CrawlerTrieNode* root, uint16_t* count) {
    if (root == NULL || count == NULL) return false;

    *count = 0;

    CrawlerQueue queue;
    crawler_queue_init(&queue);
    crawler_queue_create(&queue, 8);

    size_t offset = 0;
    CrawlerTrieNode* node = root;
    do {
        node->node_offset = offset++;
        if (*count == UINT16_MAX) {
            crawler_queue_destroy(&queue);
            return false;
        }
        (*count)++;
        for (size_t i = 0; i < CRAWLER_KEY_SPACE_SIZE; i++) {
            if (node->children[i] != NULL) {
                crawler_enqueue(&queue, node->children[i]);
            }
        }
        node = crawler_dequeue(&queue);
    } while(node != NULL);

    crawler_queue_destroy(&queue);
    return true;
}

static bool write_short(FILE* out, uint16_t value) {
    uint8_t bytes[] = {
        (uint8_t)(value >> 8),
        (uint8_t)value
    };

    return fwrite(bytes, sizeof *bytes, sizeof bytes / sizeof *bytes, out) == sizeof bytes / sizeof *bytes;
}

static bool write_int(FILE* out, uint32_t value) {
    uint8_t bytes[] = {
        bytes[0] = (uint8_t)(value >> 24),
        bytes[1] = (uint8_t)(value >> 16),
        bytes[2] = (uint8_t)(value >> 8),
        bytes[3] = (uint8_t)value
    };

    return fwrite(bytes, sizeof *bytes, sizeof bytes / sizeof *bytes, out) == sizeof bytes / sizeof *bytes;
}

static bool write_bool(FILE* out, uint8_t value) {
    return fputc(value ? 1 : 0, out) != EOF;
}

static bool serialize_node(FILE* out, const CrawlerTrieNode* node) {
    if (!write_int(out, node->char_ref.first))
        return false;

    if (!write_int(out, node->char_ref.second))
        return false;

    for (size_t i = 0; i < CRAWLER_KEY_SPACE_SIZE; i++) {
        uint16_t offset = 0;

        if (node->children[i] != NULL)
            offset = node->children[i]->node_offset;

        if (!write_short(out, offset))
            return false;
    }

    return write_bool(out, node->is_terminal);
}

bool crawler_trie_bft_serialize(const char* path, CrawlerTrieNode* root) {
    if (path == NULL || root == NULL) return false;

    FILE* out = fopen(path, "wb");
    if (out == NULL)
        return false;

    CrawlerQueue queue;
    crawler_queue_init(&queue);

    uint16_t node_count;
    if (!crawler_trie_bft_offset(root, &node_count))
        goto cleanup;
    if (!write_short(out, node_count))
        goto cleanup;

    if (!crawler_queue_create(&queue, 8))
        goto cleanup;

    CrawlerTrieNode* node = root;
    do {
        if (!serialize_node(out, node))
            goto cleanup;
        for (size_t i = 0; i < CRAWLER_KEY_SPACE_SIZE; i++) {
            if (node->children[i] != NULL) {
                crawler_enqueue(&queue, node->children[i]);
            }
        }
        node = crawler_dequeue(&queue);
    } while(node != NULL);

cleanup:
    crawler_queue_destroy(&queue);
    if (fclose(out) == EOF) return false;
    return true;
}

static bool read_short(FILE* in, uint16_t* value) {
    uint8_t bytes[2];
    if (fread(bytes, sizeof *bytes, sizeof bytes / sizeof *bytes, in) != sizeof bytes / sizeof *bytes)
        return false;

    *value = ((uint32_t)bytes[0] << 8) |
             (uint32_t)bytes[1];
    return true;
}

static bool read_int(FILE* in, uint32_t* value) {
    uint8_t bytes[4];
    if (fread(bytes, sizeof *bytes, sizeof bytes / sizeof *bytes, in) != sizeof bytes / sizeof *bytes)
        return false;

    *value = ((uint32_t)bytes[0] << 24) |
             ((uint32_t)bytes[1] << 16) |
             ((uint32_t)bytes[2] << 8)  |
             (uint32_t)bytes[3];
    return true;
}

static bool read_bool(FILE* in, uint8_t* value) {
    int read = fgetc(in);
    if (read == EOF)
        return false;
    *value = (uint8_t)read;
    return true;
}

bool crawler_static_trie_deserialize(const char* path, CrawlerStaticTrie* t) {
    if (path == NULL || t == NULL) return false;

    uint16_t* node_count = &t->node_count;
    CrawlerStaticTrieNode** root = &t->data;

    FILE* in = fopen(path, "rb");
    if (in == NULL)
        return false;

    bool result = false;

    if (!read_short(in, node_count))
        goto cleanup;

    *root = _crawler_alloc((*node_count)*sizeof **root);
    if (*root == NULL)
        goto cleanup;

    for (size_t i = 0; i < (*node_count); i++) {
        if (!read_int(in, &(*root)[i].char_ref.first))
            goto cleanup;
        if (!read_int(in, &(*root)[i].char_ref.second))
            goto cleanup;
        for (size_t j = 0; j < CRAWLER_KEY_SPACE_SIZE; j++) {
            if (!read_short(in, &(*root)[i].children_offsets[j]))
                goto cleanup;
        }
        if (!read_bool(in, (uint8_t*)&(*root)[i].is_terminal))
            goto cleanup;
    }

    result = true;
cleanup:
    if (fclose(in) == EOF) return false;
    return result;
}

bool crawler_static_trie_query(const CrawlerStaticTrie* t, const char* literal, size_t length, CrawlerCharacterReference* output) {
    if (t == NULL || literal == NULL || output == NULL) return false;
    if (!(length > 0)) return false;

    const CrawlerStaticTrieNode* curr = t->data;
    for (size_t depth = 0; depth < length; depth++) {
        size_t index = crawler_char_index(literal[depth]);
        if (index == -1) return false;
        size_t child_offset = curr->children_offsets[index];
        if (child_offset == 0) return false;
        curr = &t->data[child_offset];
    }

    if (curr->is_terminal) {
        *output = curr->char_ref;
        return true;
    }

    return false;
}