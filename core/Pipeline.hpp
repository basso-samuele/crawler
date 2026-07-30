#pragma once

#include <cstddef>

#include "TransactionalStream.hpp"
#include "Patterns.hpp"
#include "Definitions.hpp"

namespace Crawler
{

template<typename T, typename Y>
class Stage
{
private:
    Stream<T>& p_In;
    TransactionalStream<Y>& p_Out;

public:
    Stage(Stream<T>& in, TransactionalStream<Y>& out)
    : p_In(in), p_Out(out) { }

    virtual void Process() = 0;
};

class Preprocessor : public Stage<std::byte, char32_t>
{
private:

public:
    void Process();
};

}