#pragma once

#include <memory>
#include <cstdint>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <cmath>

namespace Crawler
{

constexpr size_t _MaskBitOffset = 10;
constexpr size_t _Size = 1<<_MaskBitOffset;
constexpr size_t _Mask = _Size-1;

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

public:
    InputStream();
    virtual ~InputStream() = default;

    InputStream(const InputStream&) = delete;
    InputStream& operator=(const InputStream&) = delete;

    InputStream(InputStream&&) = delete;
    InputStream& operator=(InputStream&&) = delete;

    int Peek(std::byte* const destination);
    int Seek(size_t offset);
    void Drop();

    virtual bool End() const = 0;
    virtual bool Bad() const = 0;

    virtual bool Empty() const;
};

}