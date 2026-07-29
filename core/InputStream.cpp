#include "InputStream.hpp"

#include <cstddef>

namespace Crawler
{

InputStream::InputStream()
: p_Size(_Size), p_HeadOffset(0), p_TailOffset(0), p_PeekOffset(0), p_Base(std::make_unique<std::byte[]>(_Size)) { }

int InputStream::Peek(std::byte* const destination) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    /* Suspends execution if all bytes have been picked and the input stream may still produce new ones. */
    this->p_Readable.wait(lock, [this] {
        bool isInputStreamAvailable = !this->Bad() && !this->End();
        bool allBytesPeeked = this->p_PeekOffset == this->p_TailOffset;

        return !(allBytesPeeked && isInputStreamAvailable);
    });

    bool allBytesPeeked = this->p_PeekOffset == this->p_TailOffset;
    if (allBytesPeeked) {
        return -1;
    }

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

    if (this->p_HeadOffset != this->p_PeekOffset) {
        this->p_HeadOffset = this->p_PeekOffset;
        this->p_Writable.notify_one();
    }
}

bool InputStream::Empty() const {
    return this->p_HeadOffset == this->p_TailOffset;
}

}