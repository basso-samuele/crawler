#include "Buffer.hpp"

#include <cstring>

namespace Crawler
{

void Buffer::PeekInternal(std::byte* destination, size_t* readBytes, size_t count, std::unique_lock<std::mutex>& lock) {
    this->p_CanRead.wait(lock, [this] {
        return !this->p_InternalBuffer.empty();
    });

    size_t bytesToRead = std::min(count, this->p_InternalBuffer.size());
    *readBytes = bytesToRead;

    std::memcpy(destination, this->p_InternalBuffer.data(), bytesToRead * sizeof(std::byte));
}

void Buffer::Write(const std::byte* source, size_t count) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    size_t necessarySize = this->p_InternalBuffer.size() + count;
    if (necessarySize > this->p_InternalBuffer.capacity()) {
        this->p_InternalBuffer.reserve(necessarySize);
    }

    this->p_InternalBuffer.insert(this->p_InternalBuffer.end(), source, source + count);
    this->p_CanRead.notify_one();
}

void Buffer::Read(std::byte* destination, size_t* readBytes, size_t count) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    this->PeekInternal(destination, readBytes, count, lock);
    this->p_InternalBuffer.erase(this->p_InternalBuffer.begin(), this->p_InternalBuffer.begin() + *readBytes);
}

void Buffer::Peek(std::byte* destination, size_t* readBytes, size_t count) {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    this->PeekInternal(destination, readBytes, count, lock);
}

size_t Buffer::GetSize() {
    std::unique_lock<std::mutex> lock(this->p_Mutex);

    return this->p_InternalBuffer.size();
}

}