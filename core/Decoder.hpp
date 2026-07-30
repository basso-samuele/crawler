#pragma once

#include <cstddef>

#include "TransactionalStream.hpp"

namespace Crawler
{

class UTF8Decoder
{
private:
    bool p_Sequence;
    size_t p_SequenceLength;
    size_t p_SequenceLengthValidity;
    uint32_t p_Codepoint;

    bool p_IsValidCodepoint();

public:
    UTF8Decoder();

    bool Decode(std::byte value, TransactionalStream<char32_t>& out);
    void ResetDecoderState();
};

}