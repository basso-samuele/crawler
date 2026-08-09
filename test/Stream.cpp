#include <thread>
#include <stdio.h>
#include <iostream>

#include <Stream.hpp>
#include <FileStream.hpp>

#include <pipeline/Decoder.hpp>
#include <pipeline/Preprocessor.hpp>
#include <pipeline/Tokenizer.hpp>
#include <pipeline/Tree.hpp>

namespace Stream
{

void Producer(Crawler::FileStream& inputStream) {
    while (inputStream.ReadFromDisk());
}

void Consumer(Crawler::Stream<std::byte>& inputStream) {
    Crawler::TransactionalStream<char32_t> decoded(16);
    Crawler::UTF8Decoder decoder(inputStream, decoded);

    Crawler::TransactionalStream<char32_t> normalized(16);
    Crawler::Preprocessor preprocessor(decoded, normalized);

    Crawler::TransactionalStream<Crawler::Token> tokens(16);
    Crawler::Tokenizer tokenizer(normalized, tokens);

    Crawler::TransactionalStream<std::shared_ptr<Crawler::Node>> nodes(2);
    Crawler::TreeBuilder treeBuilder(tokens, nodes);

    while (!normalized.End()) {
        decoder.Process();
        preprocessor.Process();
        tokenizer.Process();
        treeBuilder.Process();
        treeBuilder.Finalize();
    }

    std::shared_ptr<Crawler::Node> root;
    if (nodes.Peek(&root)) {
        Crawler::Utils::PrintTree(root);
    } else {
        throw "Could not build tree";
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