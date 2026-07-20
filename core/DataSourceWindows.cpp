#include "DataSource.hpp"

#define NOMINMAX
#include <Windows.h>

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
    HANDLE winFileHandle;
    HANDLE winMappingHandle;
};

FileDataSource::FileDataSource(const std::filesystem::path& path)
: p_Path(path), p_MappedMemory(nullptr), p_FileSize(0), p_IsMapped(false), p_ReadingPosition(0) {
    this->p_FileHandle = new FileHandle({ INVALID_HANDLE_VALUE, nullptr });
    this->MapFileToMemory();
}

FileDataSource::~FileDataSource() {
    if (this->p_MappedMemory != nullptr) {
        UnmapViewOfFile(this->p_MappedMemory);
    }
    if (this->p_FileHandle->winMappingHandle != nullptr) {
        CloseHandle(this->p_FileHandle->winMappingHandle);
    }
    if (this->p_FileHandle->winFileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(this->p_FileHandle->winFileHandle);
    }

    free(this->p_FileHandle);
}

bool FileDataSource::MapFileToMemory() {
    if (this->p_IsMapped) {
        return true;
    }

    this->p_FileHandle->winFileHandle = CreateFileW(
        this->p_Path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );
    if (this->p_FileHandle->winFileHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(this->p_FileHandle->winFileHandle, &fileSize)) {
        CloseHandle(this->p_FileHandle->winFileHandle);
        this->p_FileHandle->winFileHandle = INVALID_HANDLE_VALUE;
        return false;
    }
    this->p_FileSize = static_cast<size_t>(fileSize.QuadPart);

    this->p_FileHandle->winMappingHandle = CreateFileMappingW(
        this->p_FileHandle->winFileHandle,
        nullptr,
        PAGE_READONLY,
        0,
        0,
        nullptr
    );
    if (this->p_FileHandle->winMappingHandle == nullptr) {
        CloseHandle(this->p_FileHandle->winFileHandle);
        this->p_FileHandle->winFileHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    this->p_MappedMemory = MapViewOfFile(
        this->p_FileHandle->winMappingHandle,
        FILE_MAP_READ,
        0,
        0,
        0
    );

    if (this->p_MappedMemory == nullptr) {
        CloseHandle(this->p_FileHandle->winMappingHandle);
        this->p_FileHandle->winMappingHandle = nullptr;
        CloseHandle(this->p_FileHandle->winFileHandle);
        this->p_FileHandle->winFileHandle = INVALID_HANDLE_VALUE;
        return false;
    }

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