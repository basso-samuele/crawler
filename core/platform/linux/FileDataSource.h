#pragma once

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <cstddef>

#include "DataSource.h"

namespace Crawler
{

    class FileDataSource : public DataSource
    {
    private:
        std::filesystem::path p_Path;
        int p_FD;
        void* p_MappedMemory;
        size_t p_FileSize;
        bool p_IsMapped;
        size_t p_ReadingPosition;

    public:
        FileDataSource(const FileDataSource& _) = delete;
        FileDataSource& operator=(const FileDataSource& _) = delete;

        FileDataSource(FileDataSource&& _) = delete;
        FileDataSource& operator=(FileDataSource&& _) = delete;

        FileDataSource(const std::filesystem::path& path);
        ~FileDataSource();

        bool MapFileToMemory();

        virtual void Read(std::byte* destination, size_t* readBytes, size_t count) override;
        virtual void Peek(std::byte* destination, size_t* readBytes, size_t count) override;
    };

}