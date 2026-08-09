#pragma once

#include <memory>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <cmath>
#include <vector>

namespace Crawler
{

template <typename T>
class Stream
{
protected:
    std::vector<T> p_Base;
    size_t p_HeadOffset;
    size_t p_PeekOffset;

public:
    Stream()
    : p_PeekOffset(0), p_HeadOffset(0) { }
    virtual ~Stream() = default;

    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

    Stream(Stream&&) = delete;
    Stream& operator=(Stream&&) = delete;

    virtual bool Peek(T* const destination) {
        bool allBytesPeeked = this->p_PeekOffset == this->p_Base.size();
        if (allBytesPeeked) {
            return false;
        }

        (*destination) = this->p_Base[this->p_PeekOffset++];

        return true;
    }

    virtual bool Drop() {
        if (this->p_HeadOffset != this->p_PeekOffset) {
            this->p_HeadOffset = this->p_PeekOffset;
            return true;
        }
        return false;
    }

    virtual void Reset() {
        this->p_PeekOffset = this->p_HeadOffset;
    }

    virtual bool End() const = 0;
    virtual bool Bad() const = 0;

    virtual bool Empty() {
        return this->p_HeadOffset == this->p_Base.size();
    }
};

}