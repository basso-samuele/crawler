#pragma once
#include "Test.hpp"

#include <string_buffer.h>

namespace String
{

void StringBufferRelocate() {
    CrawlerString str;
    crawler_string_init(&str);
    crawler_string_create(&str, 2);
    crawler_string_append(&str, 1);
    CRAWLER_ASSERT_EQ(1, str.length);
    CRAWLER_ASSERT_EQ(str.data[0], 1);
    crawler_string_append(&str, 2);
    CRAWLER_ASSERT_EQ(2, str.length);
    CRAWLER_ASSERT_EQ(2, str.capacity);
    CRAWLER_ASSERT_EQ(str.data[0], 1);
    CRAWLER_ASSERT_EQ(str.data[1], 2);
    crawler_string_append(&str, 3);
    CRAWLER_ASSERT_EQ(3, str.length);
    CRAWLER_ASSERT_EQ(3, str.capacity);
    CRAWLER_ASSERT_EQ(str.data[0], 1);
    CRAWLER_ASSERT_EQ(str.data[1], 2);
    CRAWLER_ASSERT_EQ(str.data[2], 3);
    crawler_string_destroy(&str);
}

void StringBufferClone() {
    CrawlerString str;
    crawler_string_init(&str);
    crawler_string_create(&str, 2);
    crawler_string_append(&str, 'a');
    crawler_string_append(&str, 'b');
    CrawlerString clone;
    crawler_string_clone(&clone, &str);
    CRAWLER_ASSERT_TRUE(crawler_string_compare(&str, &clone));
    const char* reference = "ab";
    CRAWLER_ASSERT_TRUE(crawler_string_compare_with_literal(&str, const_cast<char*>(reference), 2));
    CRAWLER_ASSERT_MEMEQ(str.data, clone.data, str.length*sizeof(int));
    crawler_string_destroy(&str);
    crawler_string_destroy(&clone);
}

void Test() {
    StringBufferRelocate();
    StringBufferClone();
}

}