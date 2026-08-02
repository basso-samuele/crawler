#include "Test.hpp"

#include "IOTest.hpp"
#include "DecoderTest.hpp"
#include "TransactionalStream.hpp"

#include "Definitions.hpp"

int main(int argc, char** argv) {
    IO::Test();
    Decoder::Test();
    CRAWLER_PRINT_TEST_SUMMARY;
    return CRAWLER_TEST_RESULT;
}