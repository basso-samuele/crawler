#pragma once

#include "Stream.hpp"

#include "Definitions.hpp"

#include <array>
#include <vector>
#include <span>

namespace Crawler
{

enum class MatcherPolicy
{
    MANDATORY,
    OPTIONAL
};

template <typename T>
class Matcher
{
private:
    const MatcherPolicy p_Policy;

    const T p_Value;
    const T* p_Set;
    const size_t p_Size;

public:
    template<size_t N>
    Matcher(MatcherPolicy policy, const std::array<T, N>& set)
    : p_Policy(policy), p_Value(T{ }), p_Set(set.data()), p_Size(set.size()) {}

    template<typename Y> requires requires(Y y) { static_cast<T>(y); }
    Matcher(MatcherPolicy policy, Y value)
    : p_Policy(policy), p_Value(static_cast<T>(value)), p_Set(&this->p_Value), p_Size(1) { }

    std::tuple<MatcherPolicy, bool> Match(const T& value) {
        auto it = std::find(this->p_Set, this->p_Set + this->p_Size, value);
        bool match = it != this->p_Set + this->p_Size;
        return { this->p_Policy, match };
    }
};

enum class MatchResult
{
    PENDING, TRUE, FALSE
};

template <typename T>
class MatcherSequence
{
private:
    std::vector<Matcher<T>> p_Elements;
    size_t p_Current;

public:
    MatcherSequence(std::initializer_list<Matcher<T>> elements)
    : p_Elements(elements), p_Current(0) { }

    MatchResult Match(const Stream<T>& in) {
        const T value;
        if (!in.Peek(&value)) {
            return MatchResult::PENDING;
        }
        Matcher<T>& el = this->p_Elements.at(this->p_Current);
        auto [policy, match] = el.Match(value);

        if (!match && policy == MatcherPolicy::MANDATORY) {
            this->p_Current = 0;
            return MatchResult::FALSE;
        }

        this->p_Current++;
        if (this->p_Current == this->p_Elements.size()) {
            this->p_Current = 0;
            return MatchResult::TRUE;
        }

        return MatchResult::PENDING;
    }
};

namespace Patterns
{

inline constexpr MatcherPolicy M = MatcherPolicy::MANDATORY;
inline constexpr MatcherPolicy O = MatcherPolicy::OPTIONAL;

inline const MatcherSequence<std::byte> UTF16BEBOM({
    { M, 0xFE },
    { M, 0xFF }
});

inline const MatcherSequence<std::byte> UTF16LEBOM({
    { M, 0xFF },
    { M, 0xFE }
});

inline const MatcherSequence<std::byte> UTF8BOM({
    { M, 0xEF },
    { M, 0xBB },
    { M, 0xBF }
});

inline const MatcherSequence<std::byte> UTF16BEXML({
    { M, 0x00 },
    { M, 0x3C },
    { M, 0x00 },
    { M, 0x3F },
    { M, 0x00 },
    { M, 0x78 }
});

inline const MatcherSequence<std::byte> UTF16LEXML({
    { M, 0x3C },
    { M, 0x00 },
    { M, 0x3F },
    { M, 0x00 },
    { M, 0x78 },
    { M, 0x00 }
});

inline const MatcherSequence<std::byte> OPENCOMMENT({
    { M, 0x3C },
    { M, 0x21 },
    { M, 0x2D },
    { M, 0x2D }
});

inline const MatcherSequence<std::byte> CLOSECOMMENT({
    { M, 0x2D },
    { M, 0x2D },
    { M, 0x3E }
});

inline const MatcherSequence<std::byte> OPENMETA({
    { M, 0x3C },
    { M, CRAWLER_M },
    { M, CRAWLER_E },
    { M, CRAWLER_T },
    { M, CRAWLER_A },
    { M, CRAWLER_S }
});

inline const MatcherSequence<std::byte> THREEC({
    { M, 0x3C }, 
    { O, 0x2F },
    { M, CRAWLER_LETTERS }
});

inline const MatcherSequence<std::byte> ESQ({
    { M, 0x3C },
    { M, CRAWLER_ESQ }
});

inline const MatcherSequence<std::byte> CLOSETAG({
    { M, 0x3E }
});

inline const MatcherSequence<std::byte> SKIPSEQUENCE({
    { M, SKIP_SEQUENCE }
});

inline const MatcherSequence<std::byte> GETATTRSKIP({
    { M, CRAWLER_S }
});

inline const MatcherSequence<std::byte> GETATTREQ({
    { M, 0x3D }
});

inline const MatcherSequence<std::byte> GETATTRIFSPACES({
    { M, CRAWLER_ONE_BYTE_SPACE }
});

inline const MatcherSequence<std::byte> GETATTRBYTEABORT({
    { M, ABORT_BYTES }
});

inline const MatcherSequence<std::byte> GETATTRMAIUSCLETTERS({
    { M, CRAWLER_UPPERCASE_LETTERS }
});

inline const MatcherSequence<std::byte> GETATTRQUOTE({
    { M, QUOTES }
});

inline const MatcherSequence<std::byte> XMLOPENTAG({
    { M, 0x3C },
    { M, 0x3F },
    { M, 0x78 },
    { M, 0x6D },
    { M, 0x6C }
});

inline const MatcherSequence<std::byte> XMLENCODING({
    { M, 0x65 },
    { M, 0x6E },
    { M, 0x63 },
    { M, 0x6F },
    { M, 0x64 },
    { M, 0x69 },
    { M, 0x6E },
    { M, 0x67 }
});

inline const MatcherSequence<std::byte> XMLG({
    { M, 0x67 }
});

inline const MatcherSequence<std::byte> XMLSPACEORCONTROL({
    { M, CRAWLER_SPACE_CONTROL }
});

inline const MatcherSequence<std::byte> SPACESANDSEMICOLON({
    { M, SPACE_AND_SEMICOLON }
});

}

}