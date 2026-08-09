#include <thread>
#include <stdio.h>
#include <iostream>

#include <Stream.hpp>
#include <FileStream.hpp>

#include <pipeline/Decoder.hpp>
#include <pipeline/Preprocessor.hpp>
#include <pipeline/Tokenizer.hpp>
#include <pipeline/Tree.hpp>
#include <Timing.hpp>

int main(int argc, char** argv) {
    Crawler::FileStream fileInputStream(CRAWLER_STREAM_ASSET_PATH);

    Crawler::TransactionalStream<char32_t> decoded;
    Crawler::UTF8Decoder decoder(fileInputStream, decoded);

    Crawler::TransactionalStream<char32_t> normalized;
    Crawler::Preprocessor preprocessor(decoded, normalized);

    Crawler::TransactionalStream<Crawler::Token> tokens;
    Crawler::Tokenizer tokenizer(normalized, tokens);

    Crawler::TransactionalStream<std::shared_ptr<Crawler::Node>> nodes;
    Crawler::TreeBuilder treeBuilder(tokens, nodes);

    while (!normalized.End()) {
        decoder.Process();
        preprocessor.Process();
        tokenizer.Process();
        treeBuilder.Process();
    }

    // std::shared_ptr<Crawler::Node> root;
    // if (nodes.Peek(&root)) {
    //     Crawler::Utils::PrintTree(root);
    // } else {
    //     throw "Could not build tree";
    // }

    return 0;
}