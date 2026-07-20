#pragma once

#include <mutex>
#include <vector>
#include <condition_variable>

namespace Crawler
{

class Buffer
{
private:
    std::mutex p_Mutex;
    std::condition_variable p_CanRead;

    std::vector<std::byte> p_InternalBuffer;

private:
    void PeekInternal(std::byte* destination, size_t* readBytes, size_t count, std::unique_lock<std::mutex>& lock);

public:
    void Write(const std::byte* source, size_t count);
    void Read(std::byte* destination, size_t* readBytes, size_t count);
    void Peek(std::byte* destination, size_t* readBytes, size_t count);
    size_t GetSize();
};

}