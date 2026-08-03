#pragma once

#include "Stream.hpp"

#include "Definitions.hpp"

#include <array>
#include <vector>
#include <span>

namespace Crawler
{

namespace Sequence
{

enum class Policy
{
    MANDATORY,
    OPTIONAL
};

template <typename T>
class Matcher
{
private:
    const Policy p_Policy;

    const T p_Value;
    const T* p_Set;
    const size_t p_Size;

public:
    template<size_t N>
    constexpr Matcher(Policy policy, const std::array<T, N>& set)
    : p_Policy(policy), p_Value(T{ }), p_Set(set.data()), p_Size(set.size()) {}

    template<typename Y> requires requires(Y y) { static_cast<T>(y); }
    constexpr Matcher(Policy policy, Y value)
    : p_Policy(policy), p_Value(static_cast<T>(value)), p_Set(&this->p_Value), p_Size(1) { }

    std::tuple<Policy, bool> Match(const T& value) const {
        auto it = std::find(this->p_Set, this->p_Set + this->p_Size, value);
        bool match = it != this->p_Set + this->p_Size;
        return { this->p_Policy, match };
    }
};

enum class Result
{
    PENDING,
    TRUE,
    FALSE
};

template <typename T>
class Sequence
{
private:
    const Matcher<T>* p_Elements;
    const size_t p_Size;

    size_t p_Current;

public:
    template<size_t N>
    constexpr Sequence(const std::array<Matcher<T>, N>& elements)
    : p_Elements(elements.data()), p_Size(elements.size()), p_Current(0) { }

    Result Match(T& value) {
        const Matcher<T>& el = this->p_Elements[this->p_Current];
        auto [policy, match] = el.Match(value);

        if (!match && policy == Policy::MANDATORY) {
            this->p_Current = 0;
            return Result::FALSE;
        }

        this->p_Current++;
        if (this->p_Current == this->p_Size) {
            this->p_Current = 0;
            return Result::TRUE;
        }

        return Result::PENDING;
    }

    void Reset() {
        this->p_Current = 0;
    }
};

inline constexpr Policy M = Policy::MANDATORY;
inline constexpr Policy O = Policy::OPTIONAL;

inline constexpr std::array<Matcher<std::byte>, 2> UTF16BEBOM({
    { M, 0xFE },
    { M, 0xFF }
});

inline constexpr std::array<Matcher<std::byte>, 2> UTF16LEBOM({
    { M, 0xFF },
    { M, 0xFE }
});

inline constexpr std::array<Matcher<std::byte>, 3> UTF8BOM({
    { M, 0xEF },
    { M, 0xBB },
    { M, 0xBF }
});

inline constexpr std::array<Matcher<std::byte>, 6> UTF16BEXML({
    { M, 0x00 },
    { M, 0x3C },
    { M, 0x00 },
    { M, 0x3F },
    { M, 0x00 },
    { M, 0x78 }
});

inline constexpr std::array<Matcher<std::byte>, 6> UTF16LEXML({
    { M, 0x3C },
    { M, 0x00 },
    { M, 0x3F },
    { M, 0x00 },
    { M, 0x78 },
    { M, 0x00 }
});

inline constexpr std::array<Matcher<std::byte>, 4> OPENCOMMENT({
    { M, 0x3C },
    { M, 0x21 },
    { M, 0x2D },
    { M, 0x2D }
});

inline constexpr std::array<Matcher<std::byte>, 3> CLOSECOMMENT({
    { M, 0x2D },
    { M, 0x2D },
    { M, 0x3E }
});

inline constexpr std::array<Matcher<std::byte>, 6> OPENMETA({
    { M, 0x3C },
    { M, CRAWLER_M },
    { M, CRAWLER_E },
    { M, CRAWLER_T },
    { M, CRAWLER_A },
    { M, CRAWLER_S }
});

inline constexpr std::array<Matcher<std::byte>, 3> THREEC({
    { M, 0x3C }, 
    { O, 0x2F },
    { M, CRAWLER_LETTERS }
});

inline constexpr std::array<Matcher<std::byte>, 2> ESQ({
    { M, 0x3C },
    { M, CRAWLER_ESQ }
});

inline constexpr std::array<Matcher<std::byte>, 1> CLOSETAG({
    { M, 0x3E }
});

inline constexpr std::array<Matcher<std::byte>, 1> SKIPSEQUENCE({
    { M, SKIP_SEQUENCE }
});

inline constexpr std::array<Matcher<std::byte>, 1> GETATTRSKIP({
    { M, CRAWLER_S }
});

inline constexpr std::array<Matcher<std::byte>, 1> GETATTREQ({
    { M, 0x3D }
});

inline constexpr std::array<Matcher<std::byte>, 1> GETATTRIFSPACES({
    { M, CRAWLER_ONE_BYTE_SPACE }
});

inline constexpr std::array<Matcher<std::byte>, 1> GETATTRBYTEABORT({
    { M, ABORT_BYTES }
});

inline constexpr std::array<Matcher<std::byte>, 1> GETATTRMAIUSCLETTERS({
    { M, CRAWLER_UPPERCASE_LETTERS }
});

inline constexpr std::array<Matcher<std::byte>, 1> GETATTRQUOTE({
    { M, QUOTES }
});

inline constexpr std::array<Matcher<std::byte>, 5> XMLOPENTAG({
    { M, 0x3C },
    { M, 0x3F },
    { M, 0x78 },
    { M, 0x6D },
    { M, 0x6C }
});

inline constexpr std::array<Matcher<std::byte>, 8> XMLENCODING({
    { M, 0x65 },
    { M, 0x6E },
    { M, 0x63 },
    { M, 0x6F },
    { M, 0x64 },
    { M, 0x69 },
    { M, 0x6E },
    { M, 0x67 }
});

inline constexpr std::array<Matcher<std::byte>, 1> XMLG({
    { M, 0x67 }
});

inline constexpr std::array<Matcher<std::byte>, 1> XMLSPACEORCONTROL({
    { M, CRAWLER_SPACE_CONTROL }
});

inline constexpr std::array<Matcher<std::byte>, 1> SPACESANDSEMICOLON({
    { M, SPACE_AND_SEMICOLON }
});

inline constexpr std::array<Matcher<char32_t>, 1> SURROGATES({
    { M, CRAWLER_SURROGATES }
});

inline constexpr std::array<Matcher<char32_t>, 1> NON_CHARACTER({
    { M, CRAWLER_NON_CHARACTER }
});

inline constexpr std::array<Matcher<char32_t>, 1> CONTROL({
    { M, CRAWLER_CONTROL }
});

inline constexpr std::array<Matcher<char32_t>, 2> CRLF({
    { M, CRAWLER_CR },
    { M, CRAWLER_LF }
});

inline constexpr std::array<Matcher<char32_t>, 1> CR({
    { M, CRAWLER_CR }
});

}

}