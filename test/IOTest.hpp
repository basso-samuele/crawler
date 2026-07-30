#pragma once

#include <cstddef>
#include <string>

#include "Test.hpp"
#include "Utils.hpp"

#include "FileInputStream.hpp"

namespace IO
{

constexpr size_t _MaskBitOffset = 2;

void WriteAndReadOneByte() {
    std::byte _content{ 0x15 };
    std::string content(reinterpret_cast<char*>(&_content), sizeof(_content));
    Test::File f(content);

    Crawler::FileInputStream is(f.GetPath(), _MaskBitOffset);
    is.ReadFromDisk();

    std::byte byteFromDisk;
    CRAWLER_ASSERT_TRUE(is.Peek(&byteFromDisk));
    CRAWLER_ASSERT_TRUE((_content == byteFromDisk));
}

void Test() {
    WriteAndReadOneByte();
}

}