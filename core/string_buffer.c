#include "string_buffer.h"
#include "utils.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>

static void assert_valid(const CrawlerString* string) {
    if (string == NULL) return;
    if (string->data == NULL) {
        assert(string->length == 0);
        assert(string->capacity == 0);
    } else {
        assert(string->capacity > 0);
        assert(string->length <= string->capacity);
    }
}

#ifdef CRAWLER_DEBUG
#   define CRAWLER_CHECK_INV(string) assert_valid(string)
#else
#   define CRAWLER_CHECK_INV(string) ((void)0)
#endif

static bool relocate(CrawlerString* string, size_t required) {
    assert(string != NULL && string->data != NULL);
    if (required <= string->capacity) return true;
    size_t increment = string->capacity/4 > 0 ?
                       string->capacity/4 : 1;
    // Overflow check for automatic string capacity increase.
    if (string->capacity > SIZE_MAX - increment) return false;
    size_t new_capacity = string->capacity + increment > required ?
                          string->capacity + increment : required;
    // When multiplying by sizeof *string->data new_capacity may still overflow.
    if (new_capacity > SIZE_MAX / sizeof *string->data) return false;
    int* new_data = _crawler_alloc(new_capacity*sizeof *string->data);
    if (new_data == NULL) return false;
    // Invariant: length <= capacity.
    // I don't see why I would check for overflows when computing string->length*sizeof *string->data.
    memcpy(new_data, string->data, string->length*sizeof *string->data);
    _crawler_free(string->data);
    string->data = new_data;
    string->capacity = new_capacity;
    CRAWLER_CHECK_INV(string);
    return true;
}

void crawler_string_init(CrawlerString* string) {
    // No need to check invariant here.
    if (string == NULL) return;
    string->data = NULL;
    string->capacity = 0;
    string->length = 0;
    CRAWLER_CHECK_INV(string);
}

bool crawler_string_create(CrawlerString* string, size_t capacity) {
    CRAWLER_CHECK_INV(string);
    if (string == NULL || !(capacity > 0)) return false;
    if (string->data != NULL) return false;
    // Overflow check.
    if (capacity > SIZE_MAX / sizeof *string->data) return false;
    string->data = _crawler_alloc(capacity*sizeof *string->data);
    if (string->data == NULL) return false;
    string->capacity = capacity;
    string->length = 0;
    CRAWLER_CHECK_INV(string);
    return true;
}

void crawler_string_destroy(CrawlerString* string) {
    CRAWLER_CHECK_INV(string);
    if (string == NULL) return;
    if (string->data != NULL)
        _crawler_free(string->data);
    crawler_string_init(string);
    CRAWLER_CHECK_INV(string);
}

bool crawler_string_clone(CrawlerString* destination, const CrawlerString* source) {
    CRAWLER_CHECK_INV(destination);
    if (destination == NULL || source == NULL) return false;
    if (destination->data != NULL || source->data == NULL) return false;
    // Overflow check.
    if (source->capacity > SIZE_MAX / sizeof *destination->data) return false;
    destination->data = _crawler_alloc(source->capacity*sizeof *destination->data);
    if (destination->data == NULL) return false;
    destination->length = source->length;
    destination->capacity = source->capacity;
    memcpy(destination->data, source->data, source->length*sizeof *destination->data);
    CRAWLER_CHECK_INV(destination);
    return true;
}

bool crawler_string_append(CrawlerString* string, int cp) {
    CRAWLER_CHECK_INV(string);
    if (string == NULL) return false;
    // Refusing append to unallocated string.
    if (string->data == NULL) return false;
    if (!(string->length < string->capacity)) {
        if (!relocate(string, string->length+1)) return false;
    }
    string->data[string->length] = cp;
    string->length++;
    CRAWLER_CHECK_INV(string);
    return true;
}

bool crawler_string_compare(const CrawlerString* lhs, const CrawlerString* rhs) {
    if (lhs == NULL || rhs == NULL) return false;
    if (lhs->data == NULL || rhs->data == NULL) return false;
    if (lhs->length != rhs->length) return false;
    return !memcmp(lhs->data, rhs->data, lhs->length*sizeof *lhs->data);
}

bool crawler_string_compare_with_literal(const CrawlerString* string, const char* literal, size_t length) {
    if (string == NULL || literal == NULL) return false;
    if (string->data == NULL || !(string->length > 0)) return false;
    for (size_t i = 0; i < length && i < string->length; i++) {
        if ((int)literal[i] != string->data[i]) return false;
    }
    return true;
}

bool crawler_string_append_string_buffer(CrawlerString* destination, const CrawlerString* source) {
    CRAWLER_CHECK_INV(destination);
    if (destination == NULL || source == NULL) return false;
    if (destination->data == NULL || source->data == NULL) return false;
    if (destination->data == source->data ) return false;
    if (destination->capacity - destination->length < source->length) {
        size_t required = source->length + destination->length;
        if (source->length > SIZE_MAX - destination->length) return false;
        if (!relocate(destination, required)) return false;
    }
    memcpy(destination->data + destination->length, source->data, source->length*sizeof *destination->data);
    destination->length += source->length;
    CRAWLER_CHECK_INV(destination);
    return true;
}