#include <thread>
#include <stdio.h>
#include <iostream>

#include <Stream.hpp>
#include <FileStream.hpp>

#include <Decoder.hpp>
#include <Pipeline.hpp>
#include <Tokenizer.hpp>

namespace Stream
{

void Producer(Crawler::FileStream& inputStream) {
    while (inputStream.ReadFromDisk());
}

void Consumer(Crawler::Stream<std::byte>& inputStream) {
    Crawler::TransactionalStream<char32_t> decoded(16);
    Crawler::UTF8Decoder decoder(inputStream, decoded);

    Crawler::TransactionalStream<char32_t> normalized(12);
    Crawler::Preprocessor preprocessor(decoded, normalized);

    Crawler::TransactionalStream<Crawler::Token> tokens(8);
    Crawler::Tokenizer tokenizer(normalized, tokens);

    while (!normalized.End()) {
        decoder.Process();
        preprocessor.Process();
        tokenizer.Process();
    }
}

}

constexpr size_t _MaskBitOffset = 10;

int main(int argc, char** argv) {
    Crawler::FileStream fileInputStream(CRAWLER_STREAM_ASSET_PATH, _MaskBitOffset);
    std::thread prod(Stream::Producer, std::ref(fileInputStream));
    std::thread cons(Stream::Consumer, std::ref(fileInputStream));
    prod.join();
    cons.join();
    return 0;
}