#pragma once

#include <cstddef>

#include "TransactionalStream.hpp"
#include "Stream.hpp"
#include "Stage.hpp"

namespace Crawler
{

class UTF8Decoder : public Stage<std::byte, char32_t>
{
private:
    bool p_Sequence;
    size_t p_SequenceLength;
    size_t p_SequenceLengthValidity;
    uint32_t p_Codepoint;

    bool p_IsValidCodepoint();

public:
    UTF8Decoder(Stream<std::byte>& in, TransactionalStream<char32_t>& out);

    void Process();
    bool Decode(std::byte value, TransactionalStream<char32_t>& out);
    void ResetDecoderState();
};

}