#include "FileInputStream.hpp"

#include <filesystem>

namespace Crawler
{

FileInputStream::FileInputStream(const std::filesystem::path& filename)
: p_InputStream(filename, std::ios::binary | std::ios::in), p_End(false), p_Bad(false) { }

void FileInputStream::p_ReadChunkFromDisk(size_t count) {
    char* const destination = reinterpret_cast<char*>(this->p_Base.get() + this->p_TailOffset);
    this->p_InputStream.read(destination, count);
    this->p_TailOffset = (this->p_TailOffset + this->p_InputStream.gcount()) & _Mask;
    this->p_End = this->p_InputStream.eof();
    this->p_Bad = this->p_InputStream.bad() || this->p_InputStream.fail();
}

void FileInputStream::ReadFromDisk() {
    std::unique_lock<std::mutex> lock(this->p_Mutex);
    this->p_Writable.wait(lock, [this] {
        size_t fullBufferTailOffset = (this->p_HeadOffset-1) & _Mask;
        return (this->p_TailOffset != fullBufferTailOffset) && this->p_InputStream.good();
    });

    if (this->p_HeadOffset > this->p_TailOffset) {
        this->p_ReadChunkFromDisk(this->p_HeadOffset - this->p_TailOffset);
    } else {
        this->p_ReadChunkFromDisk(this->p_Size - this->p_TailOffset);
        this->p_ReadChunkFromDisk(this->p_HeadOffset);
    }

    this->p_Readable.notify_one();
}

bool FileInputStream::End() const {
    return this->p_End;
}

bool FileInputStream::Bad() const {
    return this->p_Bad;
}

}