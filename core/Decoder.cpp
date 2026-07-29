#include "Decoder.hpp"

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

UTF8Decoder::UTF8Decoder()
: p_Sequence(false), p_SequenceLength(0), p_SequenceLengthValidity(0), p_Codepoint(0) { }

bool UTF8Decoder::Decode(std::byte value, Sink<char32_t>& sink) {
    uint8_t intValue = static_cast<uint8_t>(value);

    if (this->p_SequenceLength > 0) {
        if ((intValue & 0xC0) != 0x80) {
            return false;
        }
        this->p_Codepoint = (this->p_Codepoint << 6) | (intValue & 0x3F);
        this->p_SequenceLength--;
    } else {
        if (this->p_Sequence) {
            if (!this->p_IsValidCodepoint()) {
                return false;
            }
            sink.Push(static_cast<char32_t>(this->p_Codepoint));
            this->p_Sequence = false;
            this->p_SequenceLength = 0;
            this->p_SequenceLengthValidity = 0;
            this->p_Codepoint = 0;
            return true;
        }

        if (intValue < 0x80) {
            sink.Push(static_cast<char32_t>(intValue));
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
            return false;
        }

        this->p_Sequence = true;
    }

    return true;
}

}