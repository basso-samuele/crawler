#pragma once

#include <filesystem>
#include <cstddef>
#include <fstream>

#include "Stream.hpp"

namespace Crawler
{

class FileStream : public Stream<std::byte>
{
private:
    std::mutex p_Mutex;
    std::condition_variable p_Readable;
    std::condition_variable p_Writable;

    std::fstream p_InputStream;
    bool p_End;
    bool p_Bad;

private:
    void p_ReadChunkFromDisk(size_t count);

public:
    FileStream(const std::filesystem::path& filename, const size_t maskBitOffset);
    ~FileStream() = default;

    FileStream(const FileStream&) = delete;
    FileStream& operator=(const FileStream&) = delete;

    FileStream(FileStream&&) = delete;
    FileStream& operator=(FileStream&&) = delete;

    bool Peek(std::byte* const destination) override;
    bool Drop() override;
    bool Empty() override;

    bool ReadFromDisk();

    bool End() const override;
    bool Bad() const override;
};

}