#include "Test.hpp"
#include "Utils.hpp"

#include <Patterns.hpp>

namespace Sequence
{

void SequenceMatchUTF16BEBOM() {
    Test::InitializedStream<std::byte> is(2, std::array<std::byte, 2>{ std::byte(0xFE), std::byte(0xFF) });
    const auto& sequenceData = Crawler::Sequence::UTF16BEBOM;
    Crawler::Sequence::Sequence<std::byte> sequence(sequenceData);
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::TRUE));
}

void SequenceMatchOPENMETA() {
    Test::InitializedStream<std::byte> is(3, std::array<std::byte, 6>{ std::byte(0x3C), std::byte(0x4D), std::byte(0x65), std::byte(0x54), std::byte(0x61), std::byte(0x2F) });
    const auto& sequenceData = Crawler::Sequence::OPENMETA;
    Crawler::Sequence::Sequence<std::byte> sequence(sequenceData);
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::TRUE));
}

void SequenceMatchNonEnoughBufferSpace() {
    Test::InitializedStream<std::byte> is(0, std::array<std::byte, 6>{ std::byte(0x3C), std::byte(0x4D), std::byte(0x65), std::byte(0x54), std::byte(0x61), std::byte(0x2F) });
    const auto& sequenceData = Crawler::Sequence::OPENMETA;
    Crawler::Sequence::Sequence<std::byte> sequence(sequenceData);
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
    CRAWLER_ASSERT_TRUE((sequence.Match(is) == Crawler::Sequence::Result::PENDING));
}

void Test() {
    SequenceMatchUTF16BEBOM();
    SequenceMatchOPENMETA();
    SequenceMatchNonEnoughBufferSpace();
}

}