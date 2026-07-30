#pragma once

#include <memory>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <cmath>

namespace Crawler
{

template <typename T>
class Stream
{
protected:
    std::unique_ptr<T[]> p_Base;
    size_t p_HeadOffset;
    size_t p_TailOffset;
    size_t p_PeekOffset;
    size_t p_Size;

    size_t p_MaskBitOffset;
    size_t p_Mask;

public:
    Stream(const size_t maskBitOffset)
    : p_MaskBitOffset(maskBitOffset), p_Size(1 << maskBitOffset), p_Mask((1 << maskBitOffset) - 1)
    , p_HeadOffset(0), p_TailOffset(0), p_PeekOffset(0), p_Base(std::make_unique<T[]>(1 << maskBitOffset)) { }
    virtual ~Stream() = default;

    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    Stream(Stream&&) = delete;
    Stream& operator=(Stream&&) = delete;

    virtual bool Peek(T* const destination) {
        bool allBytesPeeked = this->p_PeekOffset == this->p_TailOffset;
        if (allBytesPeeked) {
            return false;
        }

        (*destination) = this->p_Base[this->p_PeekOffset];
        this->p_PeekOffset = (this->p_PeekOffset + 1) & this->p_Mask;

        return true;
    }

    virtual bool Drop() {
        if (this->p_HeadOffset != this->p_PeekOffset) {
            this->p_HeadOffset = this->p_PeekOffset;
            return true;
        }
        return false;
    }

    virtual bool End() const = 0;
    virtual bool Bad() const = 0;

    virtual bool Empty() {
        return this->p_HeadOffset == this->p_TailOffset;
    }
};

}