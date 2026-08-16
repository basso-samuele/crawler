#ifndef CRAWLER_ATTRIBUTES_H_
#define CRAWLER_ATTRIBUTES_H_

#include "string_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CrawlerInternalAttribute {
    CrawlerString name;
    CrawlerString value;
} CrawlerAttribute;

void crawler_attribute_init(CrawlerAttribute* attribute);
void crawler_attribute_create(CrawlerAttribute* attribute);
void crawler_attribute_destroy(CrawlerAttribute* attribute);

typedef struct CrawlerInternalAttributeNode {
    struct CrawlerInternalAttribute attribute;
    struct CrawlerInternalAttributeNode* next;
} CrawlerAttributeNode;

void crawler_attribute_node_init(CrawlerAttributeNode* node);
void crawler_attribute_node_create(CrawlerAttributeNode* node);
void crawler_attribute_node_destroy(CrawlerAttributeNode* node);

void crawler_attribute_list_insert(CrawlerAttributeNode** root, CrawlerAttributeNode* node);
void crawler_attribute_list_destroy(CrawlerAttributeNode** root);

#ifdef __cplusplus
}
#endif
#endif