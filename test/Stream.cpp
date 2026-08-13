#include "Test.hpp"

#include <crawler.h>

int main(int argc, char** argv) {
    std::string input("<div><p>This is an ASCII string!</div></p>");
    CrawlerBuffer buffer;
    buffer.base = (unsigned char*)input.data();
    buffer.size = input.length();

    crawler_parse_buffer(&buffer);
    return 0;
}