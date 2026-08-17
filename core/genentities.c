#include "utils.h"
#include "trie.h"
#include <string.h>
#include <cJSON.h>

static CrawlerTrieNode* generate_trie(cJSON* root) {
    if (root == NULL)
        return NULL;

    CrawlerTrieNode* t = NULL;

    cJSON *entry = NULL;

    cJSON_ArrayForEach(entry, root) {
        const char *name = entry->string;

        cJSON *codepoints = cJSON_GetObjectItemCaseSensitive(
            entry, "codepoints");

        if (!cJSON_IsArray(codepoints))
            continue;

        int cp1 = 0;
        int cp2 = 0;

        cJSON *cp = cJSON_GetArrayItem(codepoints, 0);
        if (cJSON_IsNumber(cp))
            cp1 = cp->valueint;

        cp = cJSON_GetArrayItem(codepoints, 1);
        if (cJSON_IsNumber(cp))
            cp2 = cp->valueint;

        size_t namelen = strlen(name);
        size_t len = name[namelen-1] == ';' ?
                     namelen-2 : namelen-1;
        crawler_trie_insert(&t, name+1, len, cp1, cp2);
    }

    return t;
}

int main(int argc, char** argv) {
    char* raw = crawler_read_file(CRAWLER_FILE_ENTITIES);
    if (raw == NULL)
        return -1;

    cJSON* root = cJSON_Parse(raw);
    _crawler_free(raw);

    if (root == NULL || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return -1;
    }

    CrawlerTrieNode* t = generate_trie(root);
    cJSON_Delete(root);
    if (!t)
        return -1;

    if (!crawler_trie_bft_serialize(CRAWLER_CHAR_REF_TRIE, t)) {
        crawler_trie_destroy(&t);
        return -1;
    }
    crawler_trie_destroy(&t);
    return 0;
}