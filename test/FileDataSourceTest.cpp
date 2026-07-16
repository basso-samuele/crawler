#include <filesystem>
#include <fstream>
#include <random>
#include <cassert>

#include <iostream>

#include "../source/FileDataSource.h"

static std::filesystem::path GenerateUniquePath() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    std::filesystem::path dir = std::filesystem::temp_directory_path();
    std::filesystem::path fullPath;
    uint64_t name;

    do {
        name = dist(gen);
        fullPath = dir / std::to_string(name);
    } while(std::filesystem::exists(fullPath));

    return fullPath;
}

class File
{
private:
    std::filesystem::path p_Path;

public:
    File(const File& _) = delete;
    File& operator=(const File& _) = delete;

    File(File&& _) = delete;
    File& operator=(File&& _) = delete;

    File(const std::string& content)
    : p_Path(GenerateUniquePath()) {
        std::ofstream ofs(this->p_Path, std::ios::binary);
        if (ofs) {
            ofs.write(content.data(), content.size());
            ofs.flush();
        }
    }

    ~File() {
        if (!this->p_Path.empty()) {
            std::error_code ec;
            std::filesystem::remove(this->p_Path, ec);
        }
    }

    const std::filesystem::path GetPath() const {
        return this->p_Path;
    }
};

int normalReading() {
    const std::string content =
        "<html>"
            "<head></head>"
            "<body>"
                "<div><p>Hello, world!</p></div>"
            "</body>"
        "</html>";

    File temp(content);

    const std::byte* byteData = reinterpret_cast<const std::byte*>(content.data());
    const size_t size = content.size();

    Crawler::FileDataSource fileDataSource(temp.GetPath());
    std::byte byteDataFromDisk[size];
    size_t readBytes;

    fileDataSource.Read(byteDataFromDisk, &readBytes, size);

    assert(size == readBytes);
    assert(std::equal(byteDataFromDisk, byteDataFromDisk + readBytes, byteData, byteData + size));

    return 0;
}

int normalPeek() {
    const std::string content =
        "<html>"
            "<head></head>"
            "<body>"
                "<div><p>Hello, world!</p></div>"
            "</body>"
        "</html>";

    File temp(content);

    const std::byte* byteData = reinterpret_cast<const std::byte*>(content.data());
    const size_t size = content.size();

    Crawler::FileDataSource fileDataSource(temp.GetPath());
    std::byte byteDataFromDisk[size];
    size_t readBytes;

    fileDataSource.Peek(byteDataFromDisk, &readBytes, size);

    assert(size == readBytes);
    assert(std::equal(byteDataFromDisk, byteDataFromDisk + readBytes, byteData, byteData + size));

    return 0;
}

int readNilCount() {
    const std::string content =
        "<html>"
            "<head></head>"
            "<body>"
                "<div><p>Hello, world!</p></div>"
            "</body>"
        "</html>";

    File temp(content);

    const std::byte* byteData = reinterpret_cast<const std::byte*>(content.data());
    const size_t size = content.size();

    Crawler::FileDataSource fileDataSource(temp.GetPath());
    std::byte byteDataFromDisk[size];
    size_t readBytes;

    const size_t nil = 0;

    fileDataSource.Read(byteDataFromDisk, &readBytes, nil);
    assert(readBytes == nil);

    fileDataSource.Peek(byteDataFromDisk, &readBytes, nil);
    assert(readBytes == nil);

    return 0;
}

int readWithWrongCount() {
    const std::string content =
        "<html>"
            "<head></head>"
            "<body>"
                "<div><p>Hello, world!</p></div>"
            "</body>"
        "</html>";

    File temp(content);

    const std::byte* byteData = reinterpret_cast<const std::byte*>(content.data());
    const size_t size = content.size();

    Crawler::FileDataSource fileDataSource(temp.GetPath());
    std::byte byteDataFromDisk[size];
    size_t readBytes;

    fileDataSource.Read(byteDataFromDisk, &readBytes, size * 2);

    assert(size == readBytes);
    assert(std::equal(byteDataFromDisk, byteDataFromDisk + readBytes, byteData, byteData + size));

    return 0;
}

int emptyFile() {
    const std::string content = "";

    File temp(content);

    const size_t size = content.size();

    Crawler::FileDataSource fileDataSource(temp.GetPath());
    std::byte byteDataFromDisk[size];
    size_t readBytes;

    fileDataSource.Read(byteDataFromDisk, &readBytes, 10);

    assert(0 == readBytes);

    return 0;
}

int main(int argc, char** argv) {
    int testResult = 0;
    testResult |= normalReading();
    testResult |= normalPeek();
    testResult |= readNilCount();
    testResult |= readWithWrongCount();
    testResult |= emptyFile();
    return testResult;
}