#include "FileDataSource.h"

#include <cstring>
#include <algorithm>

namespace Crawler
{

    FileDataSource::FileDataSource(const std::filesystem::path& path)
    : p_Path(path), p_FileHandle(INVALID_HANDLE_VALUE), p_MappingHandle(nullptr), p_MappedMemory(nullptr), p_FileSize(0), p_IsMapped(false), p_ReadingPosition(0) { }

    FileDataSource::~FileDataSource() {
        if (this->p_MappedMemory != nullptr) {
            UnmapViewOfFile(this->p_MappedMemory);
            this->p_MappedMemory = nullptr;
        }
        if (this->p_MappingHandle != nullptr) {
            CloseHandle(this->p_MappingHandle);
            this->p_MappingHandle = nullptr;
        }
        if (this->p_FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(this->p_FileHandle);
            this->p_FileHandle = INVALID_HANDLE_VALUE;
        }
        this->p_IsMapped = false;
        this->p_FileSize = 0;
    }

    bool FileDataSource::MapFileToMemory() {
        if (this->p_IsMapped) {
            return true;
        }

        this->p_FileHandle = CreateFileW(
            this->p_Path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr
        );

        if (this->p_FileHandle == INVALID_HANDLE_VALUE) {
            return false;
        }

        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(this->p_FileHandle, &fileSize)) {
            CloseHandle(this->p_FileHandle);
            p_FileHandle = INVALID_HANDLE_VALUE;
            return false;
        }
        this->p_FileSize = static_cast<size_t>(fileSize.QuadPart);

        this->p_MappingHandle = CreateFileMappingW(
            this->p_FileHandle,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr
        );

        if (this->p_MappingHandle == nullptr) {
            CloseHandle(this->p_FileHandle);
            this->p_FileHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        this->p_MappedMemory = MapViewOfFile(
            this->p_MappingHandle,
            FILE_MAP_READ,
            0,
            0,
            0
        );

        if (this->p_MappedMemory == nullptr) {
            CloseHandle(this->p_MappingHandle);
            this->p_MappingHandle = nullptr;
            CloseHandle(this->p_FileHandle);
            this->p_FileHandle = INVALID_HANDLE_VALUE;
            return false;
        }

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