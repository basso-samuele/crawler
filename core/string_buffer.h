#ifndef CRAWLER_STRING_H_
#define CRAWLER_STRING_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Represents a unicode string. It may be used to store tag names and attributes.
 * The structure holds a pointer to a heap allocated memory location, may improve
 * with SSO in the future.
 * 
 * Invalid string state (different from empty):
 *      data == NULL -> lenght == 0 and capacity == 0
 * Valid string state (may be empty):
 *      data != NULL -> capacity > 0 and length <= capacity
 * 
 * An empty string:
 *      data != NULL and length == 0
 * 
 * In debug builds class invariants are verified before after each manipulation.
 */
typedef struct {
    int* data;
    size_t length;
    size_t capacity;
} CrawlerString;

// Initializes the structure to the invalid string state.
void crawler_string_init(CrawlerString* string);

// Should be preceded by init/destroy. If a valid string is passed then the call will have no effect.
// 
// Allocates a unicode string with capacity.
bool crawler_string_create(CrawlerString* string, size_t capacity);

// Releases allocated resources and initializes the structure to the invalid string state.
void crawler_string_destroy(CrawlerString* string);

// It does what you think it does if destination has no allocated resources and source is a valid string.
bool crawler_string_clone(CrawlerString* destination, const CrawlerString* source);

// Appends cp to string. String is relocated if necessary.
bool crawler_string_append(CrawlerString* string, int cp);

bool crawler_string_compare(const CrawlerString* lhs, const CrawlerString* rhs);
bool crawler_string_compare_with_literal(const CrawlerString* string, const char* literal, size_t length);

// Necessary to emit the temporary buffer as a series of character tokens.
bool crawler_string_append_string_buffer(CrawlerString* destination, const CrawlerString* source);

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_STRING_H_