#include "FileStream.hpp"

#include <filesystem>
#include <algorithm>

#include "Stream.hpp"

namespace Crawler
{

FileStream::FileStream(const std::filesystem::path& filename)
: p_End(false), p_Bad(false) {
    std::fstream is(filename, std::ios::binary | std::ios::in);
    std::for_each(
        std::istreambuf_iterator<char>(is),
        std::istreambuf_iterator<char>(),
        [&](const char c){
            this->p_Base.push_back(static_cast<std::byte>(c));
        }
    );
}

bool FileStream::End() const {
    return true;
}

bool FileStream::Bad() const {
    return false;
}

}