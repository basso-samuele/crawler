#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Test.hpp"
#include "Utils.hpp"

#include "FileInputStream.hpp"

namespace IO
{

constexpr size_t _MaskBitOffset = 2;

struct IOTest {
    std::vector<std::byte> input;
};

void RunIOTest(const IOTest& test) {
    std::string content = Test::BSToString(test.input);
    Test::File f(content);
    Crawler::FileInputStream is(f.GetPath(), _MaskBitOffset);
    is.ReadFromDisk();

    std::vector<std::byte> actual;
    for (std::byte b; is.Peek(&b); is.Drop(), is.ReadFromDisk()) actual.push_back(b);
    CRAWLER_ASSERT_EQ(test.input.size(), actual.size());
    CRAWLER_ASSERT_MEMEQ(test.input.data(), actual.data(), test.input.size());
}

const std::vector<IOTest> ioTests({
    { Test::BS(0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF) },
    { Test::BS(0x0, 0x1, 0x2, 0x3) },
    { Test::BS(0x0) },
    { Test::BS() }
});

void Test() {
    for (const IOTest& t : ioTests) {
        RunIOTest(t);
    }
}

}