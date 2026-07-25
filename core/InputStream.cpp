#include "InputStream.hpp"

#include <cstddef>

namespace Crawler
{

InputStream::InputStream()
: p_Size(_Size), p_HeadOffset(0), p_TailOffset(0), p_PeekOffset(0), p_Base(std::make_unique<std::byte[]>(_Size)) { }

int InputStream::Peek(std::byte* const destination) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);
    this->p_Readable.wait(lock, [this] {
        return this->p_PeekOffset != this->p_TailOffset;
    });

    (*destination) = this->p_Base[this->p_PeekOffset];
    this->p_PeekOffset = (this->p_PeekOffset + 1) & _Mask;

    return 0;
}

int InputStream::Seek(size_t offset) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    if (offset > this->p_PeekOffset) {
        return -1;
    }

    this->p_PeekOffset = offset;
    return 0;
}

void InputStream::Drop() {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    this->p_HeadOffset = this->p_PeekOffset;
    this->p_PeekOffset = 0;

    this->p_Writable.notify_one();
}

}