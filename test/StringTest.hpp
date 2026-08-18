#pragma once

#include "gtest/gtest.h"
#include <string_buffer.h>

namespace
{

class String : public testing::Test
{
protected:
    void SetUp() override {
        crawler_string_init(&string);
        ASSERT_TRUE(crawler_string_create(&string, 2));
    }

    void TearDown() override {
        crawler_string_destroy(&string);
    }

    CrawlerString string;
};

TEST_F(String, Append) {
    crawler_string_append(&string, 1);
    ASSERT_EQ(1, string.length);
    ASSERT_EQ(string.data[0], 1);
    crawler_string_append(&string, 2);
    ASSERT_EQ(2, string.length);
    ASSERT_EQ(2, string.capacity);
    ASSERT_EQ(string.data[0], 1);
    ASSERT_EQ(string.data[1], 2);
    crawler_string_append(&string, 3);
    ASSERT_EQ(3, string.length);
    ASSERT_EQ(3, string.capacity);
    ASSERT_EQ(string.data[0], 1);
    ASSERT_EQ(string.data[1], 2);
    ASSERT_EQ(string.data[2], 3);
    crawler_string_destroy(&string);
}

TEST_F(String, Compare) {
    CrawlerString clone;
    crawler_string_init(&clone);

    ASSERT_TRUE(crawler_string_append(&string, 'a'));
    ASSERT_TRUE(crawler_string_append(&string, 'b'));
    ASSERT_TRUE(crawler_string_clone(&clone, &string));
    ASSERT_TRUE(crawler_string_compare(&string, &clone));
    const char* reference = "ab";
    ASSERT_TRUE(crawler_string_compare_with_literal(&string, reference, 2));
    ASSERT_TRUE(memcmp(string.data, clone.data, string.length*sizeof *string.data) == 0);

    crawler_string_destroy(&clone);
}

}