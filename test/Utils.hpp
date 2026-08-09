#pragma once

#include <filesystem>
#include <concepts>
#include <vector>
#include <cstddef>
#include <algorithm>

#include <Stream.hpp>

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

template <typename T>
class InitializedStream : public Crawler::Stream<T>
{
public:
    template <size_t N>
    InitializedStream(std::array<T, N> data) {
        size_t count = data.size();
        size_t size = count * sizeof(T);
        // copy data into vector
    }
    virtual ~InitializedStream() = default;

    InitializedStream(const InitializedStream&) = delete;
    InitializedStream& operator=(const InitializedStream&) = delete;

    InitializedStream(InitializedStream&&) = delete;
    InitializedStream& operator=(InitializedStream&&) = delete;

    virtual bool End() const {
        return true;
    }

    virtual bool Bad() const {
        return true;
    }
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