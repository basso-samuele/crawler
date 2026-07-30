#pragma once

#include <memory>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <cmath>

namespace Crawler
{

/**
 * The size is fixed and non-configurable. At this stage the implementation works only if the internal buffer size
 * is a power of 2.
 * 
 * @see InputStream::Peek
 */
class InputStream
{
protected:
    std::mutex p_Mutex;
    std::condition_variable p_Readable;
    std::condition_variable p_Writable;

    std::unique_ptr<std::byte[]> p_Base;
    size_t p_HeadOffset;
    size_t p_TailOffset;
    size_t p_PeekOffset;
    size_t p_Size;

    size_t p_MaskBitOffset;
    size_t p_Mask;

public:
    InputStream(const size_t maskBitOffset);
    virtual ~InputStream() = default;

    InputStream(const InputStream&) = delete;
    InputStream& operator=(const InputStream&) = delete;

    InputStream(InputStream&&) = delete;
    InputStream& operator=(InputStream&&) = delete;

    bool Peek(std::byte* const destination);
    void Drop();

    virtual bool End() const = 0;
    virtual bool Bad() const = 0;

    virtual bool Empty() const;
};

}