#include "InputStream.hpp"

#include <cstddef>

namespace Crawler
{

InputStream::InputStream(const size_t maskBitOffset)
: p_MaskBitOffset(maskBitOffset), p_Size(1<<maskBitOffset), p_Mask((1<<maskBitOffset)-1),
  p_HeadOffset(0), p_TailOffset(0), p_PeekOffset(0), p_Base(std::make_unique<std::byte[]>(1<<maskBitOffset)) { }

bool InputStream::Peek(std::byte* const destination) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    /* Suspends execution if all bytes have been picked and the input stream may still produce new ones. */
    this->p_Readable.wait(lock, [this] {
        bool isInputStreamAvailable = !this->Bad() && !this->End();
        bool allBytesPeeked = this->p_PeekOffset == this->p_TailOffset;

        return !(allBytesPeeked && isInputStreamAvailable);
    });

    bool allBytesPeeked = this->p_PeekOffset == this->p_TailOffset;
    if (allBytesPeeked) {
        return false;
    }

    (*destination) = this->p_Base[this->p_PeekOffset];
    this->p_PeekOffset = (this->p_PeekOffset + 1) & this->p_Mask;

    return true;
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