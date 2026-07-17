#include "FileDataSource.h"

#include <cstring>
#include <algorithm>

namespace Crawler
{

    FileDataSource::FileDataSource(const std::filesystem::path& path)
    : p_Path(path), p_FD(-1), p_MappedMemory(nullptr), p_FileSize(0), p_IsMapped(false), p_ReadingPosition(0) { }

    FileDataSource::~FileDataSource() {
        if (this->p_MappedMemory != nullptr && this->p_MappedMemory != MAP_FAILED) {
            munmap(this->p_MappedMemory, this->p_FileSize);
            this->p_MappedMemory = nullptr;
        }
        if (this->p_FD != -1) {
            close(this->p_FD);
            this->p_FD = -1;
        }
        this->p_IsMapped = false;
        this->p_FileSize = 0;
    }

    bool FileDataSource::MapFileToMemory() {
        if (this->p_IsMapped) {
            return true;
        }

        this->p_FD = open(this->p_Path.c_str(), O_RDONLY);
        if (this->p_FD == -1) {
            return false;
        }

        struct stat st;
        if (fstat(this->p_FD, &st) == -1) {
            close(this->p_FD);
            this->p_FD = -1;
            return false;
        }
        this->p_FileSize = static_cast<size_t>(st.st_size);

        this->p_MappedMemory = mmap(nullptr, this->p_FileSize, PROT_READ, MAP_PRIVATE, this->p_FD, 0);
        if (this->p_MappedMemory == MAP_FAILED) {
            close(this->p_FD);
            this->p_FD = -1;
            return false;
        }

        madvise(this->p_MappedMemory, this->p_FileSize, MADV_SEQUENTIAL);

        this->p_IsMapped = true;
        return true;
    }

    void FileDataSource::Read(std::byte* destination, size_t* readBytes, size_t count) {
        this->Peek(destination, readBytes, count);
        this->p_ReadingPosition += *readBytes;
    }

    void FileDataSource::Peek(std::byte* destination, size_t* readBytes, size_t count) {
        if (!this->MapFileToMemory()) {
            *readBytes = 0;
            return;
        }

        if (this->p_ReadingPosition > this->p_FileSize) {
            *readBytes = 0;
            return;
        }

        size_t bytesToRead = std::min(count, (this->p_FileSize - this->p_ReadingPosition));
        *readBytes = bytesToRead;

        std::byte* source = static_cast<std::byte*>(this->p_MappedMemory);
        std::memcpy(destination, source + this->p_ReadingPosition, bytesToRead * sizeof(std::byte));
    }

}