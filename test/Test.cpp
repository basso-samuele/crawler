#include "Test.hpp"

#include "UTF8StreamTest.hpp"
#include "StringTest.hpp"
#include "LexerTest.hpp"
#include "QueueTest.hpp"
#include "TrieTest.hpp"

int main(int argc, char** argv) {
    Stream::Test();
    String::Test();
    // Lexer::Test();
    Queue::Test();
    Trie::Test();
    CRAWLER_PRINT_TEST_SUMMARY;
    return CRAWLER_TEST_RESULT;
}