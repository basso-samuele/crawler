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
    bool p_End;
    bool p_Bad;

private:
    void p_ReadChunkFromDisk(size_t count);

public:
    FileStream(const std::filesystem::path& filename);
    ~FileStream() = default;

    FileStream(const FileStream&) = delete;
    FileStream& operator=(const FileStream&) = delete;

    FileStream(FileStream&&) = delete;
    FileStream& operator=(FileStream&&) = delete;

    bool End() const override;
    bool Bad() const override;
};

}