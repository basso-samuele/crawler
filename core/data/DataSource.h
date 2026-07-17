#pragma once

#include <cstddef>

namespace Crawler
{

    class DataSource
    {
    public:
        virtual ~DataSource() = default;

        virtual void Read(std::byte* destination, size_t* readBytes, size_t count) = 0;
        virtual void Peek(std::byte* destination, size_t* readBytes, size_t count) = 0;
    };

}