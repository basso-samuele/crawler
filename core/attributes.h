#ifndef CRAWLER_ATTRIBUTES_H_
#define CRAWLER_ATTRIBUTES_H_

#include "string_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CrawlerString name;
    CrawlerString value;
} CrawlerAttribute;

void crawler_attribute_init(CrawlerAttribute* attribute);
void crawler_attribute_create(CrawlerAttribute* attribute);
void crawler_attribute_destroy(CrawlerAttribute* attribute);

#ifdef __cplusplus
}
#endif
#endif