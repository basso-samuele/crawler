#pragma once

#include <cstddef>

#include "Queue.hpp"

namespace Crawler
{

class Decoder
{
public:
    virtual ~Decoder() = default;

    virtual bool Decode(std::byte value, Sink<char32_t> sink) = 0;
};

class UTF8Decoder : public Decoder
{
private:
    bool p_Sequence;
    size_t p_SequenceLength;
    size_t p_SequenceLengthValidity;
    uint32_t p_Codepoint;

    bool p_IsValidCodepoint();

public:
    UTF8Decoder();

    bool Decode(std::byte value, Sink<char32_t>& sink);
};

}