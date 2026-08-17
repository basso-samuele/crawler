#include "attributes.h"
#include "utils.h"

void crawler_attribute_init(CrawlerAttribute* attribute) {
    crawler_string_init(&attribute->name);
    crawler_string_init(&attribute->value);
}

void crawler_attribute_create(CrawlerAttribute* attribute) {
    crawler_string_create(&attribute->name, 8);
    crawler_string_create(&attribute->value, 8);
}

void crawler_attribute_destroy(CrawlerAttribute* attribute) {
    crawler_string_destroy(&attribute->name);
    crawler_string_destroy(&attribute->value);
}

void crawler_attribute_node_init(CrawlerAttributeNode* node) {
    crawler_attribute_init(&node->attribute);
    node->next = NULL;
}

void crawler_attribute_node_create(CrawlerAttributeNode* node) {
    crawler_attribute_create(&node->attribute);
}

void crawler_attribute_node_destroy(CrawlerAttributeNode* node) {
    crawler_attribute_destroy(&node->attribute);
}

void crawler_attribute_list_insert(CrawlerAttributeNode** root, CrawlerAttributeNode* node) {
    node->next = *root;
    *root = node;
}

void crawler_attribute_list_destroy(CrawlerAttributeNode** root) {
    CrawlerAttributeNode* curr = *root;
    CrawlerAttributeNode* it = NULL;
    while(curr) {
        it = curr->next;
        crawler_attribute_destroy(&curr->attribute);
        _crawler_free(curr);
        curr = it;
    }
}