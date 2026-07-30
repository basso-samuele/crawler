#pragma once

#include <filesystem>
#include <concepts>
#include <vector>
#include <cstddef>

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

template <std::integral... Ts>
constexpr std::vector<std::byte> BS(Ts... values) {
    return std::vector<std::byte>{
        static_cast<std::byte>(values)...
    };
}

template <std::integral T>
constexpr std::byte B(T value) {
    return std::byte(value);
}

std::vector<std::byte> StringToBS(std::string s);
std::string BSToString(std::vector<std::byte> bs);

}