#include "Test.hpp"

#include "PatternTest.hpp"
#include "IOTest.hpp"
#include "EncodingTest.hpp"
#include "DecoderTest.hpp"
#include "TransactionalStream.hpp"

int main(int argc, char** argv) {
    Crawler::TransactionalStream<char32_t> buffer(10);

    Pattern::Test();
    IO::Test();
    Encoding::Test();
    Decoder::Test();
    CRAWLER_PRINT_TEST_SUMMARY;
    return CRAWLER_TEST_RESULT;
}