#pragma once

#include "gtest/gtest.h"

#include <vector>
#include <cstddef>
#include <cstdint>

#include <stream.h>
#include <buffer.h>
#include <parser.h>

namespace
{

struct DecodeTest {
    std::vector<unsigned char> input;
    CrawlerStreamResult result;
    int expectedCodePoint;
};

class UTF8Stream : public testing::TestWithParam<DecodeTest> {};

CrawlerBuffer CreateBuffer(const std::vector<unsigned char>& input) {
    return { (unsigned char*)input.data(), input.size(), true };
}

TEST_P(UTF8Stream, Decoding) {
    const auto& tc = GetParam();

    CrawlerBuffer buffer = CreateBuffer(tc.input);

    CrawlerParserContext parser;
    crawler_parser_init(&parser);
    crawler_parser_bind_buffer(&parser, &buffer);

    auto result = crawler_stream_get(&parser);
    ASSERT_EQ(tc.result, result);
    if (result == CRAWLER_STREAM_SUCCESS)
        ASSERT_EQ(tc.expectedCodePoint, parser.is.current_code_point);
}

INSTANTIATE_TEST_SUITE_P(
    UTF8Iterator,
    UTF8Stream,
    ::testing::Values(
        DecodeTest{ { 0x00 }, CRAWLER_STREAM_SUCCESS, 0x0000 },
        DecodeTest{ { 0x7F }, CRAWLER_STREAM_SUCCESS, 0x007F },
        DecodeTest{ { 0xC2, 0x80 }, CRAWLER_STREAM_SUCCESS, 0x0080 },
        DecodeTest{ { 0xDF, 0xBF }, CRAWLER_STREAM_SUCCESS, 0x07FF },
        DecodeTest{ { 0xE0, 0xA0, 0x80 }, CRAWLER_STREAM_SUCCESS, 0x0800 },
        DecodeTest{ { 0xEF, 0xBF, 0xBF }, CRAWLER_STREAM_SUCCESS, 0xFFFF },
        DecodeTest{ { 0xF0, 0x90, 0x80, 0x80 }, CRAWLER_STREAM_SUCCESS, 0x10000 },
        DecodeTest{ { 0xF4, 0x8F, 0xBF, 0xBF }, CRAWLER_STREAM_SUCCESS, 0x10FFFF },
        DecodeTest{ { 0xC0, 0x80 }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xC1, 0xBF }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xE0, 0x80, 0x80 }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xF0, 0x80, 0x80, 0x80 }, CRAWLER_STREAM_ERROR, 0x0 },
                /* Invalid continuation bytes. */
        DecodeTest{ { 0xC2, 0x20 }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xE2, 0x28, 0xA1 }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xF0, 0x28, 0x8C, 0xBC }, CRAWLER_STREAM_ERROR, 0x0 },
                /* Missing continuation bytes. */
        DecodeTest{ { 0xC2 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
        DecodeTest{ { 0xE2, 0x82 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
        DecodeTest{ { 0xF0, 0x90, 0x80 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
                /* Refused when the code point is deemed invalid. */
        DecodeTest{ { 0xF5 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
        DecodeTest{ { 0xF6 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
        DecodeTest{ { 0xF7 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
                /* Invalid leading bytes. */
        DecodeTest{ { 0xF8 }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xF9 }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xFA }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xFB }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xFC }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xFD }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xFE }, CRAWLER_STREAM_ERROR, 0x0 },
        DecodeTest{ { 0xFF }, CRAWLER_STREAM_ERROR, 0x0 },
                /* Normalizing new lines. */
        DecodeTest{ { 0x0D }, CRAWLER_STREAM_MISSING_ELEMENT, 0x000A },
        DecodeTest{ { 0x0D, 0x0A }, CRAWLER_STREAM_SUCCESS, 0x000A },
        DecodeTest{ { 0x0D, 0x00 }, CRAWLER_STREAM_SUCCESS, 0x000A }
    )
);

}