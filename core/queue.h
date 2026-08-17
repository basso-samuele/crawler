#ifndef CRAWLER_QUEUE_H_
#define CRAWLER_QUEUE_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Invariants:
 *      data == NULL -> size == 0 and capacity == 0
 *      data != NULL -> capacity > 0 and size <= capacity
 */
typedef struct {
    void** data;
    size_t capacity;
    size_t size;
} CrawlerQueue;

void crawler_queue_init(CrawlerQueue* queue);
bool crawler_queue_create(CrawlerQueue* queue, size_t capacity);
void crawler_queue_destroy(CrawlerQueue* queue);

bool crawler_enqueue(CrawlerQueue* queue, void* ptr);
void* crawler_dequeue(CrawlerQueue* queue);

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_QUEUE_H_