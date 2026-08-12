#include "Test.hpp"

#include "UTF8StreamTest.hpp"
#include "StringTest.hpp"

int main(int argc, char** argv) {
    Stream::Test();
    String::Test();
    CRAWLER_PRINT_TEST_SUMMARY;
    return CRAWLER_TEST_RESULT;
}