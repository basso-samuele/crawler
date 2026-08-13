#include "string_buffer.h"
#include "memory.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>

void crawler_string_destroy(CrawlerString* string) {
    if (string->data) _crawler_free((void*)string->data);
    crawler_string_init(string);
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

void crawler_string_clone(CrawlerString* destination, CrawlerString* source) {
    destination->length = source->length;
    destination->capacity = source->capacity;
    destination->data = (int*)_crawler_alloc(source->capacity*sizeof(int));
    memcpy(destination->data, source->data, source->capacity*sizeof(int));
}

void crawler_string_append_string_buffer(CrawlerString* destination, CrawlerString* source) {
    while (destination->capacity - destination->length < source->length)
        // This size increase strategy is not optimal, but for a first implementation should be fine.
        crawler_string_relocate(destination);
    memcpy(((int*)destination->data) + destination->length, source->data, source->length*sizeof(int));
    destination->length += source->length;
}

bool crawler_string_compare(CrawlerString* lhs, CrawlerString* rhs) {
    if ((lhs->length != rhs->length) || (lhs->capacity != rhs->capacity)) return false;
    return !memcmp(lhs->data, rhs->data, lhs->length);
}

bool crawler_string_compare_with_literal(CrawlerString* string, char* literal) {
    assert(string->length);
    for (size_t i = 0; i < string->length; i++) {
        if (!literal[i] || !((int)literal[i] == string->data[i]))
            return false;
    }
    return true;
}