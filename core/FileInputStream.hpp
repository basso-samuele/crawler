#pragma once

#include <filesystem>
#include <fstream>

#include "InputStream.hpp"

namespace Crawler
{

class FileInputStream : public InputStream
{
private:
    std::fstream p_InputStream;
    bool p_End;
    bool p_Bad;

private:
    void p_ReadChunkFromDisk(size_t count);

public:
    FileInputStream(const std::filesystem::path& filename);
    ~FileInputStream() = default;

    FileInputStream(const FileInputStream&) = delete;
    FileInputStream& operator=(const FileInputStream&) = delete;

    FileInputStream(FileInputStream&&) = delete;
    FileInputStream& operator=(FileInputStream&&) = delete;

    bool ReadFromDisk();

    bool End() const override;
    bool Bad() const override;
};

}