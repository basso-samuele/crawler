#include "DataSource.hpp"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <filesystem>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <cassert>

namespace Crawler
{

struct FileHandle
{
    int fd;
};

FileDataSource::FileDataSource(const std::filesystem::path& path)
: p_Path(path), p_MappedMemory(nullptr), p_FileSize(0), p_IsMapped(false), p_ReadingPosition(0) {
    this->p_FileHandle = new FileHandle({ -1 });
    this->MapFileToMemory();
}

FileDataSource::~FileDataSource() {
    if (this->p_MappedMemory != nullptr && this->p_MappedMemory != MAP_FAILED) {
        munmap(this->p_MappedMemory, this->p_FileSize);
    }
    if (this->p_FileHandle->fd != -1) {
        close(this->p_FileHandle->fd);
    }
    free(this->p_FileHandle);
}

bool FileDataSource::MapFileToMemory() {
    if (this->p_IsMapped) {
        return true;
    }

    assert(this->p_FileHandle != nullptr);

    this->p_FileHandle->fd = open(this->p_Path.c_str(), O_RDONLY);
    if (this->p_FileHandle->fd == -1) {
        return false;
    }

    struct stat st;
    if (fstat(this->p_FileHandle->fd, &st) == -1) {
        close(this->p_FileHandle->fd);
        this->p_FileHandle->fd = -1;
        return false;
    }

    this->p_FileSize = static_cast<size_t>(st.st_size);
    this->p_MappedMemory = mmap(nullptr, this->p_FileSize, PROT_READ, MAP_PRIVATE, this->p_FileHandle->fd, 0);
    if (this->p_MappedMemory == MAP_FAILED) {
        close(this->p_FileHandle->fd);
        this->p_FileHandle->fd = -1;
        return false;
    }

    madvise(this->p_MappedMemory, this->p_FileSize, MADV_SEQUENTIAL);

    this->p_IsMapped = true;
    return true;
}

void FileDataSource::Read(uint8_t* destination, size_t* readBytes, size_t count) {
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

    uint8_t* source = static_cast<uint8_t*>(this->p_MappedMemory);
    std::memcpy(destination, source + this->p_ReadingPosition, bytesToRead * sizeof(uint8_t));
    this->p_ReadingPosition += *readBytes;
}

void FileDataSource::Seek(size_t pos) {
    this->p_ReadingPosition = pos;
}

size_t FileDataSource::Size() {
    return this->p_FileSize;
}

}