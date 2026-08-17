#pragma once
#include "Test.hpp"

#include <queue.h>

namespace Queue
{

void QueueGenericTest() {
    CrawlerQueue queue;
    crawler_queue_init(&queue);
    crawler_queue_create(&queue, 1);
    int a = 0x1234;
    crawler_enqueue(&queue, &a);
    crawler_enqueue(&queue, &a);
    CRAWLER_ASSERT_EQ(2, queue.capacity);
    CRAWLER_ASSERT_EQ(2, queue.size);
    crawler_enqueue(&queue, &a);
    crawler_enqueue(&queue, &a);
    CRAWLER_ASSERT_EQ(4, queue.capacity);
    CRAWLER_ASSERT_EQ(4, queue.size);
    void* ptr = crawler_dequeue(&queue);
    CRAWLER_ASSERT_EQ(4, queue.capacity);
    CRAWLER_ASSERT_EQ(3, queue.size);
    CRAWLER_ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    CRAWLER_ASSERT_EQ(4, queue.capacity);
    CRAWLER_ASSERT_EQ(2, queue.size);
    CRAWLER_ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    CRAWLER_ASSERT_EQ(4, queue.capacity);
    CRAWLER_ASSERT_EQ(1, queue.size);
    CRAWLER_ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    CRAWLER_ASSERT_EQ(4, queue.capacity);
    CRAWLER_ASSERT_EQ(0, queue.size);
    CRAWLER_ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    CRAWLER_ASSERT_EQ(4, queue.capacity);
    CRAWLER_ASSERT_EQ(0, queue.size);
    CRAWLER_ASSERT_EQ(NULL, ptr);
}

void Test() {
    QueueGenericTest();
}

}