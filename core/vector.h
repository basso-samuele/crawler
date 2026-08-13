#ifndef CRAWLER_VECTOR_H_
#define CRAWLER_VECTOR_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void** data;
    size_t capacity;
    size_t size;
} CrawlerVector;

void crawler_vector_init(CrawlerVector* vector);
void crawler_vector_create(CrawlerVector* vector, size_t capacity);
void crawler_vector_destroy(CrawlerVector* vector);
void crawler_vector_append(CrawlerVector* vector, void* ptr);

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_VECTOR_H_