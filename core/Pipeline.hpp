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
protected:
    Stream<T>& p_In;
    TransactionalStream<Y>& p_Out;

public:
    Stage(Stream<T>& in, TransactionalStream<Y>& out)
    : p_In(in), p_Out(out) { }

    virtual void Process() = 0;
};

class Preprocessor : public Stage<char32_t, char32_t>
{
private:
    Sequence::Sequence<char32_t> p_Surrogates;
    Sequence::Sequence<char32_t> p_NonCharacters;
    Sequence::Sequence<char32_t> p_Control;
    Sequence::Sequence<char32_t> p_CRLF;
    Sequence::Sequence<char32_t> p_CR;

private:
    bool p_Sanify(char32_t currentCodePoint);
    bool p_Verify(Sequence::Sequence<char32_t>& s, char32_t currentCodePoint);

public:
    Preprocessor(Stream<char32_t>& in, TransactionalStream<char32_t>& out)
    : Stage<char32_t, char32_t>(in, out), p_Surrogates(Sequence::SURROGATES)
    , p_NonCharacters(Sequence::NON_CHARACTER), p_Control(Sequence::CONTROL)
    , p_CRLF(Sequence::CRLF), p_CR(Sequence::CR) { }

    void Process();
};

}