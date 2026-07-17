#include <stdio.h>
#include <cassert>

#include "Buffer.h"

int writeAndPeek() {
    std::byte bytes[5] = { std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5} };

    Crawler::Buffer buffer;
    buffer.Write(bytes, sizeof(bytes)/sizeof(std::byte));
    size_t countAfterInsertion = buffer.GetSize();

    size_t readBytes;
    std::byte destination[10];
    buffer.Peek(destination, &readBytes, sizeof(destination)/sizeof(std::byte));

    size_t countAfterPeek = buffer.GetSize();

    assert(countAfterInsertion == countAfterPeek);
    assert(readBytes == 5);
    assert(std::equal(std::begin(bytes), std::end(bytes), std::begin(destination)));

    return 0;
}

int writeAndRead() {
    std::byte bytes[5] = { std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5} };

    Crawler::Buffer buffer;
    buffer.Write(bytes, sizeof(bytes)/sizeof(std::byte));

    size_t readBytes;
    std::byte destination[10];
    buffer.Read(destination, &readBytes, sizeof(destination)/sizeof(std::byte));

    size_t countAfterRead = buffer.GetSize();

    assert(countAfterRead == 0);
    assert(readBytes == 5);
    assert(std::equal(std::begin(bytes), std::end(bytes), std::begin(destination)));

    return 0;
}

int main(int argc, char** argv) {
    int testResult = 0;
    testResult |= writeAndPeek();
    testResult |= writeAndRead();
    return testResult;
}