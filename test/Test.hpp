#pragma once

#include <filesystem>
#include <string>

namespace Crawler
{

namespace Test
{

std::filesystem::path GenerateUniquePath();

class File
{
private:
    std::filesystem::path p_Path;

public:
    File(const File& _) = delete;
    File& operator=(const File& _) = delete;

    File(File&& _) = delete;
    File& operator=(File&& _) = delete;

    File(const std::string& content);
    ~File();

    const std::filesystem::path GetPath() const;
};

}

}