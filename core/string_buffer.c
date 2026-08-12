#include "string_buffer.h"
#include "memory.h"

#include <assert.h>
#include <string.h>

void crawler_string_destroy(CrawlerString* string) {
    if (string->data) _crawler_free((void*)string->data);
    string->capacity = 0;
    string->length = 0;
}

void crawler_string_init(CrawlerString* string) {
    string->data = NULL;
    string->capacity = 0;
    string->length = 0;
}

void crawler_string_create(CrawlerString* string, size_t capacity) {
    assert(capacity > 0);
    string->capacity = 0;
    if (string->data = (int*)_crawler_alloc(capacity*sizeof(int)))
        string->capacity = capacity;
    string->length = 0;
}

static void crawler_string_relocate(CrawlerString* string) {
    size_t new_capacity = string->capacity * 2;
    int* new = (int*)_crawler_alloc(new_capacity*sizeof(int));
    memcpy(new, string->data, string->length*sizeof(int));
    _crawler_free((void*)string->data);
    string->data = new;
    string->capacity = new_capacity;
}

void crawler_string_append(CrawlerString* string, int cp) {
    if (!(string->length < string->capacity))
        crawler_string_relocate(string);
    assert(string->length < string->capacity);
    string->data[string->length++] = cp;
}