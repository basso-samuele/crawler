#ifndef CRAWLER_STRING_H_
#define CRAWLER_STRING_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a unicode string. It may be used to store tag names and attributes.
 * The structure holds a pointer to a heap allocated memory location, may improve
 * with SSO in the future.
 */
typedef struct {
    int* data;
    size_t length;
    size_t capacity;
} CrawlerString;

void crawler_string_init(CrawlerString* string);
void crawler_string_create(CrawlerString* string, size_t capacity);
void crawler_string_destroy(CrawlerString* string);
void crawler_string_append(CrawlerString* string, int cp);

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_STRING_H_