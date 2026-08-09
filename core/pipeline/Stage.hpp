#pragma once
#include "TransactionalStream.hpp"

namespace Crawler
{

template<typename T, typename Y>
class Stage
{
protected:
    Stream<T>& p_In;
    TransactionalStream<Y>& p_Out;

public:
    Stage(Stream<T>& in, TransactionalStream<Y>& out)
    : p_In(in), p_Out(out) { }

    virtual void Process() = 0;
};

}