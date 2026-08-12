#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

#include "Test.hpp"

#include <stream.h>
#include <buffer.h>
#include <parser.h>

namespace Stream
{

struct DecodeTest {
    std::vector<unsigned char> input;
    CrawlerStreamResult result;
    int expectedCodePoint;
};

const std::vector<DecodeTest> uft8DecoderTests({
    { { 0x00 }, CRAWLER_STREAM_SUCCESS, 0x0000 },
    { { 0x7F }, CRAWLER_STREAM_SUCCESS, 0x007F },
    { { 0xC2, 0x80 }, CRAWLER_STREAM_SUCCESS, 0x0080 },
    { { 0xDF, 0xBF }, CRAWLER_STREAM_SUCCESS, 0x07FF },
    { { 0xE0, 0xA0, 0x80 }, CRAWLER_STREAM_SUCCESS, 0x0800 },
    { { 0xEF, 0xBF, 0xBF }, CRAWLER_STREAM_SUCCESS, 0xFFFF },
    { { 0xF0, 0x90, 0x80, 0x80 }, CRAWLER_STREAM_SUCCESS, 0x10000 },
    { { 0xF4, 0x8F, 0xBF, 0xBF }, CRAWLER_STREAM_SUCCESS, 0x10FFFF },
    { { 0xC0, 0x80 }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xC1, 0xBF }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xE0, 0x80, 0x80 }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xF0, 0x80, 0x80, 0x80 }, CRAWLER_STREAM_ERROR, 0x0 },
    /* Invalid continuation bytes. */
    { { 0xC2, 0x20 }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xE2, 0x28, 0xA1 }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xF0, 0x28, 0x8C, 0xBC }, CRAWLER_STREAM_ERROR, 0x0 },
    /* Missing continuation bytes. */
    { { 0xC2 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
    { { 0xE2, 0x82 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
    { { 0xF0, 0x90, 0x80 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
    /* Refused when the code point is deemed invalid. */
    { { 0xF5 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
    { { 0xF6 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
    { { 0xF7 }, CRAWLER_STREAM_MISSING_ELEMENT, 0x0 },
    /* Invalid leading bytes. */
    { { 0xF8 }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xF9 }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xFA }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xFB }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xFC }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xFD }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xFE }, CRAWLER_STREAM_ERROR, 0x0 },
    { { 0xFF }, CRAWLER_STREAM_ERROR, 0x0 },
    /* Normalizing new lines. */
    { { 0x0D }, CRAWLER_STREAM_SUCCESS, 0x000A },
    { { 0x0D, 0x0A }, CRAWLER_STREAM_SUCCESS, 0x000A },
    { { 0x0D, 0x00 }, CRAWLER_STREAM_SUCCESS, 0x000A }
});

void UTF8StreamTest() {
    for (const DecodeTest& t : uft8DecoderTests) {
        CrawlerBuffer buffer;
        buffer.base = (unsigned char*)t.input.data();
        buffer.size = t.input.size();
        buffer.eof = true;
        CrawlerParserContext parser;
        crawler_parser_bind_buffer(&parser, &buffer);
        auto result = crawler_stream_peek(&parser);
        CRAWLER_ASSERT_EQ(static_cast<int>(t.result), static_cast<int>(result));
        if (result == CRAWLER_STREAM_SUCCESS) {
            auto codepoint = crawler_stream_current(&parser);
            CRAWLER_ASSERT_EQ(static_cast<int>(t.expectedCodePoint), static_cast<int>(codepoint));
        }
    }
}

void Test() {
    UTF8StreamTest();
}

}