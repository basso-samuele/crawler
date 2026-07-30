#include "Utils.hpp"

#include <filesystem>
#include <random>
#include <fstream>
#include <concepts>
#include <vector>
#include <cstddef>

namespace Test
{

std::filesystem::path GenerateUniquePath() {
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

File::File(const std::string& content)
: p_Path(GenerateUniquePath()) {
    std::ofstream ofs(this->p_Path, std::ios::binary);
    if (ofs) {
        ofs.write(content.data(), content.size());
        ofs.flush();
    }
}

File::~File() {
    if (!this->p_Path.empty()) {
        std::error_code ec;
        std::filesystem::remove(this->p_Path, ec);
    }
}

const std::filesystem::path File::GetPath() const {
    return this->p_Path;
}

std::vector<std::byte> StringToBS(std::string s) {
    return std::vector<std::byte>(
        reinterpret_cast<const std::byte*>(s.data()),
        reinterpret_cast<const std::byte*>(s.data() + s.size())
    );
}

std::string BSToString(std::vector<std::byte> bs) {
    return std::string(reinterpret_cast<char*>(bs.data()), bs.size());
}

}