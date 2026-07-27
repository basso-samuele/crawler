#pragma once

#include <vector>
#include <cstddef>

#include "Test.hpp"
#include "Utils.hpp"

#include "Encoding.hpp"

namespace Pattern
{

inline constexpr Crawler::ByteSpecPolicy M = Crawler::ByteSpecPolicy::MANDATORY;
inline constexpr Crawler::ByteSpecPolicy O = Crawler::ByteSpecPolicy::OPTIONAL;

void MatchSingleMandatory() {
    std::vector<std::byte> data = Test::BS(0x01, 0x02);
    Crawler::Pattern pattern({ { M, 0x01 }, { M, 0x02 } });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void MatchSetMandatory() {
    std::vector<std::byte> data = Test::BS(0x01, 0x02);
    Crawler::Pattern pattern({ { M, 0x01, 0x03 }, { M, 0x02, 0x03 } });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void MatchOptional() {
    std::vector<std::byte> data = Test::BS(0x01, 0x02);
    Crawler::Pattern pattern({ { M, 0x01, 0x02 }, { O, 0x05, 0x06 }, { M, 0x02 } });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void Test() {
    MatchSingleMandatory();
    MatchSetMandatory();
    MatchOptional();
}

}