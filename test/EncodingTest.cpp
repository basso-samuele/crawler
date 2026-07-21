#include "Encoding.hpp"

#include <cassert>

int encodingMatchExact() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { 0x01, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x02, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x03, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x04, Crawler::ByteSpecPolicy::MANDATORY }
    });
    return !pattern.Match(data.data(), data.size());
}

int encodingMatchSet() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY }
    });
    return !pattern.Match(data.data(), data.size());
}

int encodingMatchOptional() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x05, 0x06 }, Crawler::ByteSpecPolicy::OPTIONAL },
        { 0x02, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x05, 0x06 }, Crawler::ByteSpecPolicy::OPTIONAL }
    });
    return !pattern.Match(data.data(), data.size());
}

int encodingMatchRange() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
    });
    return !pattern.Match(data.data(), data.size());
}

struct AttrTest
{
    std::string input;
    std::string expectedName;
    std::string expectedValue;
    size_t expectedEndPos;
};

int runAttrTest(const AttrTest& test) {
    std::vector<uint8_t> data(test.input.begin(), test.input.end());
    size_t pos = 0;

    auto [name, value] = Crawler::GetAnAttribute(data.data(), data.size(), &pos);

    assert(name == test.expectedName);
    assert(value == test.expectedValue);
    assert(pos == test.expectedEndPos);

    return 0;
}

int testGenAnAttribute() {
    std::vector<AttrTest> tests({
        { ">", "", "", 0 },
        { " />", "", "", 2 },
        { " /", "", "", 2 },
        { " \t\n\f\r name=value", "name", "value", 16 },
        { "malf>", "malf", "", 4 },
        { "malf/>", "malf", "", 4 },
        { "/name=value", "name", "value", 11 },
        { ">rest", "", "", 0 },
        { "   >rest", "", "", 3 },
        { "/>rest", "", "", 1 },
        { "NaMe=value", "name", "value", 10 },
        { "name  =value", "name", "value", 12 },
        { "a>rest", "a", "", 1 },
        { "a=\"ABC\"", "a", "abc", 7 },
        { "a='ABC'", "a", "abc", 7 },
        { "a=v\tx", "a", "v", 3 }
    });

    int ris = 0;
    for (const AttrTest& t : tests) {
        ris |= runAttrTest(t);
    }
    return ris;
}

int main(int argc, char** argv) {
    int testResult = 0;
    testResult |= encodingMatchExact();
    testResult |= encodingMatchSet();
    testResult |= encodingMatchOptional();
    testResult |= encodingMatchRange();

    testResult |= testGenAnAttribute();

    return testResult;
}