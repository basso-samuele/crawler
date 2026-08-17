#include "queue.h"
#include "utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

static void assert_valid(const CrawlerQueue* queue) {
    if (queue == NULL) return;
    if (queue->data == NULL) {
        assert(queue->size == 0);
        assert(queue->capacity == 0);
    } else {
        assert(queue->capacity > 0);
        assert(queue->size <= queue->capacity);
    }
}

#ifdef CRAWLER_DEBUG
#   define CRAWLER_CHECK_INV(queue) assert_valid(queue)
#else
#   define CRAWLER_CHECK_INV(queue) ((void)0)
#endif

static bool relocate(CrawlerQueue* queue) {
    if (queue == NULL) return false;
    if (queue->data == NULL) return false;
    size_t increment = queue->capacity / 4 > 0 ?
                       queue->capacity / 4 : 1;
    if (queue->capacity > SIZE_MAX - increment) return false;
    size_t new_capacity = queue->capacity + increment;
    if (new_capacity > SIZE_MAX / sizeof *queue->data) return false;
    void** new_data = _crawler_alloc(new_capacity*sizeof *queue->data);
    if (new_data == NULL) return false;
    memcpy(new_data, queue->data, queue->size*sizeof *queue->data);
    _crawler_free(queue->data);
    queue->data = new_data;
    queue->capacity = new_capacity;
    return true;
}

void crawler_queue_init(CrawlerQueue* queue) {
    memset(queue, 0, sizeof *queue);
}

bool crawler_queue_create(CrawlerQueue* queue, size_t capacity) {
    if (queue == NULL || !(capacity > 0)) return false;
    if (queue->data != NULL) return false;
    // Overflow check.
    if (capacity > SIZE_MAX / sizeof *queue->data) return false;
    queue->data = _crawler_alloc(capacity*sizeof *queue->data);
    if (queue->data == NULL) return false;
    queue->capacity = capacity;
    queue->size = 0;
    CRAWLER_CHECK_INV(queue);
    return true;
}

void crawler_queue_destroy(CrawlerQueue* queue) {
    if (queue == NULL) return;
    if (queue->data == NULL) return;
    _crawler_free(queue->data);
    crawler_queue_init(queue);
    CRAWLER_CHECK_INV(queue);
}

bool crawler_enqueue(CrawlerQueue* queue, void* ptr) {
    if (queue == NULL || ptr == NULL) return false;
    if (queue->data == NULL) return false;
    if (!(queue->size < queue->capacity)) {
        if (!relocate(queue)) return false;
    }
    queue->data[queue->size] = ptr;
    queue->size++;
    CRAWLER_CHECK_INV(queue);
    return true;
}

static bool remove_first(CrawlerQueue* queue) {
    if (queue == NULL) return false;
    if (queue->data == NULL || queue->size == 0) return false;
    memmove(queue->data, queue->data+1, queue->size*sizeof *queue->data);
    queue->size--;
    return true;
}

void* crawler_dequeue(CrawlerQueue* queue) {
    if (queue == NULL) return NULL;
    if (queue->data == NULL || queue->size == 0) return NULL;
    void* result = queue->data[0];
    if (!remove_first(queue)) return NULL;
    CRAWLER_CHECK_INV(queue);
    return result;
}