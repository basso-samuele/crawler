#pragma once

#include "gtest/gtest.h"
#include <queue.h>

namespace
{

class Queue : public testing::Test
{
protected:
    void SetUp() override {
        crawler_queue_init(&queue);
        ASSERT_TRUE(crawler_queue_create(&queue, 1));
    }

    void TearDown() override {
        crawler_queue_destroy(&queue);
    }

    CrawlerQueue queue;
};

TEST_F(Queue, EnqueueDequeueOnce) {
    int a = 0x1234;
    ASSERT_TRUE(crawler_enqueue(&queue, &a));
    ASSERT_EQ(1, queue.capacity);
    ASSERT_EQ(1, queue.size);

    void* ptr = crawler_dequeue(&queue);
    ASSERT_EQ(1, queue.capacity);
    ASSERT_EQ(0, queue.size);
    ASSERT_EQ(a, *((int*)ptr));
}

TEST_F(Queue, Resize) {
    int a = 0x1234;
    crawler_enqueue(&queue, &a);
    crawler_enqueue(&queue, &a);
    ASSERT_EQ(2, queue.capacity);
    ASSERT_EQ(2, queue.size);
    crawler_enqueue(&queue, &a);
    crawler_enqueue(&queue, &a);
    ASSERT_EQ(4, queue.capacity);
    ASSERT_EQ(4, queue.size);
    void* ptr = crawler_dequeue(&queue);
    ASSERT_EQ(4, queue.capacity);
    ASSERT_EQ(3, queue.size);
    ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    ASSERT_EQ(4, queue.capacity);
    ASSERT_EQ(2, queue.size);
    ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    ASSERT_EQ(4, queue.capacity);
    ASSERT_EQ(1, queue.size);
    ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    ASSERT_EQ(4, queue.capacity);
    ASSERT_EQ(0, queue.size);
    ASSERT_EQ(a, *((int*)ptr));
    ptr = crawler_dequeue(&queue);
    ASSERT_EQ(4, queue.capacity);
    ASSERT_EQ(0, queue.size);
    ASSERT_EQ(NULL, ptr);
}

}