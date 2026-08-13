#include "vector.h"
#include "memory.h"

#include <assert.h>
#include <string.h>

void crawler_vector_init(CrawlerVector* vector) {
    vector->data = NULL;
    vector->capacity = 0;
    vector->size = 0;
}

void crawler_vector_create(CrawlerVector* vector, size_t capacity) {
    assert(capacity > 0);
    vector->capacity = 0;
    if (vector->data = _crawler_alloc(capacity*sizeof(void*)))
        vector->capacity = capacity;
    vector->size = 0;
}

void crawler_vector_destroy(CrawlerVector* vector) {
    if (vector->data) _crawler_free(vector->data);
    crawler_vector_init(vector);
}

static void crawler_vector_relocate(CrawlerVector* vector) {
    size_t new_capacity = vector->capacity * 2;
    void** new = _crawler_alloc(new_capacity*sizeof(void*));
    memcpy(new, vector->data, vector->size*sizeof(void*));
    _crawler_free(vector->data);
    vector->data = new;
    vector->capacity = new_capacity;
}

void crawler_vector_append(CrawlerVector* vector, void* ptr) {
    if (!vector->data)
        crawler_vector_create(vector, 1);
    if (!(vector->size < vector->capacity))
        crawler_vector_relocate(vector);
    assert(vector->size < vector->capacity);
    vector->data[vector->size] = ptr;
}