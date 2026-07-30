#include "Test.hpp"

#include "PatternTest.hpp"
#include "IOTest.hpp"
#include "EncodingTest.hpp"
#include "DecoderTest.hpp"

int main(int argc, char** argv) {
    Pattern::Test();
    IO::Test();
    Encoding::Test();
    Decoder::Test();
    CRAWLER_PRINT_TEST_SUMMARY;
    return CRAWLER_TEST_RESULT;
}