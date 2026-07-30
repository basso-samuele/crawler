#include "FileStream.hpp"

#include <filesystem>
#include <algorithm>

#include "Stream.hpp"

namespace Crawler
{

FileStream::FileStream(const std::filesystem::path& filename, const size_t maskBitOffset)
: Stream(maskBitOffset), p_InputStream(filename, std::ios::binary | std::ios::in), p_End(false), p_Bad(false) { }

void FileStream::p_ReadChunkFromDisk(size_t count) {
    char* const destination = reinterpret_cast<char*>(this->p_Base.get() + this->p_TailOffset);
    this->p_InputStream.read(destination, count);
    this->p_TailOffset = (this->p_TailOffset + this->p_InputStream.gcount()) & this->p_Mask;
}

bool FileStream::Peek(std::byte* const destination) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    /* Suspends execution if all bytes have been picked and the input stream may still produce new ones. */
    this->p_Readable.wait(lock, [this] {
        bool isStreamAvailable = !this->Bad() && !this->End();
        bool allBytesPeeked = this->p_PeekOffset == this->p_TailOffset;

        return !(allBytesPeeked && isStreamAvailable);
    });

    return Stream::Peek(destination);
}

bool FileStream::Drop() {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    if (Stream::Drop()) {
        this->p_Writable.notify_one();
        return true;
    }
    return false;
}

bool FileStream::Empty() {
    std::unique_lock<std::mutex> lock(this->p_Mutex);
    return Stream::Empty();
}

bool FileStream::ReadFromDisk() {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    /* Suspends execution if the buffer is currently full and the input stream has not reached its end. */
    this->p_Writable.wait(lock, [this] {
        bool isInputStreamAvailable = !this->Bad() && !this->End();

        size_t fullBufferTailOffset = (this->p_HeadOffset - 1) & this->p_Mask;
        bool isBufferFull = (this->p_TailOffset == fullBufferTailOffset);

        return !(isBufferFull && isInputStreamAvailable);
    });

    size_t fullBufferTailOffset = (this->p_HeadOffset - 1) & this->p_Mask;
    bool isBufferFull = (this->p_TailOffset == fullBufferTailOffset);
    if (this->Bad() || this->End() || isBufferFull) {
        return false;
    }

    if (this->p_HeadOffset > this->p_TailOffset) {
        this->p_ReadChunkFromDisk(this->p_HeadOffset - this->p_TailOffset - 1);
    } else {
        const auto tailChunk = this->p_Size - this->p_TailOffset;

        if (this->p_HeadOffset == 0) {
            this->p_ReadChunkFromDisk(tailChunk - 1);
        } else {
            this->p_ReadChunkFromDisk(tailChunk);
            this->p_ReadChunkFromDisk(this->p_HeadOffset - 1);
        }
    }

    this->p_Readable.notify_one();
    return true;
}

bool FileStream::End() const {
    return this->p_InputStream.eof();
}

bool FileStream::Bad() const {
    return this->p_InputStream.bad() || this->p_InputStream.fail();
}

}