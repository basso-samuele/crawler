#include "Encoding.hpp"

int encodingMatchExact() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { 0x01, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x02, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x03, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x04, Crawler::ByteSpecPolicy::MANDATORY }
    });
    return !pattern.Match(data, 0);
}

int encodingMatchSet() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY }
    });
    return !pattern.Match(data, 0);
}

int encodingMatchOptional() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x05, 0x06 }, Crawler::ByteSpecPolicy::OPTIONAL },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x05, 0x06 }, Crawler::ByteSpecPolicy::OPTIONAL }
    });
    return !pattern.Match(data, 0);
}

int encodingMatchRange() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
    });
    return !pattern.Match(data, 0);
}

int main(int argc, char** argv) {
    int testResult = 0;
    testResult |= encodingMatchExact();
    testResult |= encodingMatchSet();
    testResult |= encodingMatchOptional();
    testResult |= encodingMatchRange();
    return testResult;
}