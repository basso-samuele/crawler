#include "Decoder.hpp"
#include "Definitions.hpp"

namespace Crawler
{

bool UTF8Decoder::p_IsValidCodepoint() {
    switch (this->p_SequenceLengthValidity) {
    case 2:
        if (this->p_Codepoint < 0x80) {
            return false;
        }
        break;
    case 3:
        if (this->p_Codepoint < 0x800) {
            return false;
        }
        break;
    case 4:
        if (this->p_Codepoint < 0x10000) {
            return false;
        }
        break;
    default:
        return false;
    }

    if (this->p_Codepoint > 0x10FFFF) {
        return false;
    }

    if (this->p_Codepoint > 0xD7FF && this->p_Codepoint < 0xE000) {
        return false;
    }

    return true;
}

void UTF8Decoder::ResetDecoderState() {
    this->p_Sequence = false;
    this->p_SequenceLength = 0;
    this->p_SequenceLengthValidity = 0;
    this->p_Codepoint = 0;
}

void UTF8Decoder::Process() {
    for (std::byte b; this->p_In.Peek(&b);) {
        this->Decode(b, this->p_Out);
        this->p_In.Drop();
    }

    if (this->p_In.End()) {
        this->p_Out.SetEndFlag();
    }
}

UTF8Decoder::UTF8Decoder(Stream<std::byte>& in, TransactionalStream<char32_t>& out)
: Stage<std::byte, char32_t>(in, out), p_Sequence(false), p_SequenceLength(0), p_SequenceLengthValidity(0), p_Codepoint(0) { }

bool UTF8Decoder::Decode(std::byte value, TransactionalStream<char32_t>& out) {
    uint8_t intValue = static_cast<uint8_t>(value);

    if (this->p_SequenceLength > 0) {
        if ((intValue & 0xC0) != 0x80) {
            this->ResetDecoderState();
            return false;
        }
        this->p_Codepoint = (this->p_Codepoint << 6) | (intValue & 0x3F);
        this->p_SequenceLength--;

        if (!this->p_SequenceLength) {
            if (!this->p_IsValidCodepoint()) {
                this->ResetDecoderState();
                return false;
            }
            out.Put(static_cast<char32_t>(this->p_Codepoint));
            this->ResetDecoderState();
            return true;
        }
    } else {
        if (intValue < 0x80) {
            out.Put(static_cast<char32_t>(intValue));
            return true;
        }

        if ((intValue & 0xE0) == 0xC0) {
            this->p_SequenceLength = 2;
            this->p_SequenceLengthValidity = 2;
            this->p_Codepoint = intValue & 0x1F;
        } else if ((intValue & 0xF0) == 0xE0) {
            this->p_SequenceLength = 3;
            this->p_SequenceLengthValidity = 3;
            this->p_Codepoint = intValue & 0x0F;
        } else if ((intValue & 0xF8) == 0xF0) {
            this->p_SequenceLength = 4;
            this->p_SequenceLengthValidity = 4;
            this->p_Codepoint = intValue & 0x07;
        } else {
            this->ResetDecoderState();
            return false;
        }

        /* The byte that opens the sequence has been processed. */
        this->p_SequenceLength--;
        this->p_Sequence = true;
    }

    return true;
}

}