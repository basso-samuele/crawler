#include "Test.hpp"

#include <parser.h>
#include <buffer.h>

int main(int argc, char** argv) {
    unsigned char data[] = "<div><p>This is an ASCII string!</div></p>";
    CrawlerBuffer buffer;
    buffer.base = data;
    buffer.size = sizeof(data);

    crawler_parse(&buffer);
    return 0;
}