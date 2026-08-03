#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

#include "Test.hpp"
#include "Utils.hpp"

#include "Decoder.hpp"

namespace Decoder
{

struct DecoderTest {
    std::vector<std::byte> input;
    bool expectedCorrectness;
    /* Detection byte zero is used as a special value when errors are not expected. */
    size_t expectedDetectionByte;
    char32_t expectedCodePoint;
};

const std::vector<DecoderTest> uft8DecoderTests({
    { Test::BS(0x00), true, 0, 0x0000 },
    { Test::BS(0x7F), true, 0, 0x007F },
    { Test::BS(0xC2, 0x80), true, 0, 0x0080 },
    { Test::BS(0xDF, 0xBF), true, 0, 0x07FF },
    { Test::BS(0xE0, 0xA0, 0x80), true, 0, 0x0800 },
    { Test::BS(0xEF, 0xBF, 0xBF), true, 0, 0xFFFF },
    { Test::BS(0xF0, 0x90, 0x80, 0x80), true, 0, 0x10000 },
    { Test::BS(0xF4, 0x8F, 0xBF, 0xBF), true, 0, 0x10FFFF },
    { Test::BS(0xC0, 0x80), false, 2, 0x0 },
    { Test::BS(0xC1, 0xBF), false, 2, 0x0 },
    { Test::BS(0xE0, 0x80, 0x80), false, 3, 0x0 },
    { Test::BS(0xF0, 0x80, 0x80, 0x80), false, 4, 0x0 },
    /* Invalid continuation bytes. */
    { Test::BS(0xC2, 0x20), false, 2, 0x0 },
    { Test::BS(0xE2, 0x28, 0xA1), false, 2, 0x0 },
    { Test::BS(0xF0, 0x28, 0x8C, 0xBC), false, 2, 0x0 },
    /* Missing continuation bytes. */
    { Test::BS(0xC2), false, 0, 0x0 },
    { Test::BS(0xE2, 0x82), false, 0, 0x0 },
    { Test::BS(0xF0, 0x90, 0x80), false, 0, 0x0 },
    /* Invalid leading bytes. */
    { Test::BS(0xF5), false, 0, 0x0 },
    { Test::BS(0xF6), false, 0, 0x0 },
    { Test::BS(0xF7), false, 0, 0x0 },
    { Test::BS(0xF8), false, 1, 0x0 },
    { Test::BS(0xF9), false, 1, 0x0 },
    { Test::BS(0xFA), false, 1, 0x0 },
    { Test::BS(0xFB), false, 1, 0x0 },
    { Test::BS(0xFC), false, 1, 0x0 },
    { Test::BS(0xFD), false, 1, 0x0 },
    { Test::BS(0xFE), false, 1, 0x0 },
    { Test::BS(0xFF), false, 1, 0x0 }
});

void UTF8DecoderTest() {
    Crawler::TransactionalStream<std::byte> in(0);
    Crawler::TransactionalStream<char32_t> out(0);
    Crawler::UTF8Decoder decoder(in, out);

    for (const DecoderTest& t : uft8DecoderTests) {
        Crawler::TransactionalStream<char32_t> out(10);
        size_t detectionByte = 0;

        for (
            auto [i, correct] = std::tuple{ (size_t)0, true };
            i < t.input.size() && correct;
            i++
        ) {
            correct = decoder.Decode(t.input.at(i), out);
            if (!correct) detectionByte = i + 1;
        }

        char32_t codepoint;
        bool correctness = out.Peek(&codepoint);
        CRAWLER_ASSERT_EQ(t.expectedCorrectness, correctness);
        CRAWLER_ASSERT_EQ(t.expectedDetectionByte, detectionByte);
        if (correctness) {
            CRAWLER_ASSERT_TRUE((t.expectedCodePoint == codepoint));
        } else {
            /* When a sequence having invalid length is supplied, the decoder is left in an inconsistent state. */
            decoder.ResetDecoderState();
        }
    }
}

void Test() {
    UTF8DecoderTest();
}

}