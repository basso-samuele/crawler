#include "attributes.h"

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