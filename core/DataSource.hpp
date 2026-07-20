#pragma once

#include <cstdint>
#include <cstddef>
#include <filesystem>

namespace Crawler
{

class DataSource
{
public:
    virtual ~DataSource() = default;

    virtual void Read(uint8_t* destination, size_t* readCount, size_t count) = 0;
    virtual void Seek(size_t pos) = 0;

    virtual size_t Size() = 0;
};

struct FileHandle;

class FileDataSource : public DataSource
{
private:
    std::filesystem::path p_Path;
    void* p_MappedMemory;
    size_t p_FileSize;
    bool p_IsMapped;
    size_t p_ReadingPosition;

    FileHandle* p_FileHandle;

public:
    FileDataSource(const FileDataSource& _) = delete;
    FileDataSource& operator=(const FileDataSource& _) = delete;

    FileDataSource(FileDataSource&& _) = delete;
    FileDataSource& operator=(FileDataSource&& _) = delete;

    FileDataSource(const std::filesystem::path& path);
    ~FileDataSource();

    bool MapFileToMemory();

    virtual void Read(uint8_t* destination, size_t* readCount, size_t count) override;
    virtual void Seek(size_t pos) override;

    virtual size_t Size() override;
};

}