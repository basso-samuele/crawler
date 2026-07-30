#pragma once

#include "Encoding.hpp"
#include "../test/Utils.hpp"
#include "Stream.hpp"

namespace Crawler
{

enum class MatcherPolicy
{
    MANDATORY,
    OPTIONAL
};

template <typename T>
struct Matcher
{
    const MatcherPolicy policy;
    const std::vector<T> set;

    Matcher(MatcherPolicy policy, std::vector<T>&& set)
    : policy(policy), set(std::move(set)) { }

    Matcher(MatcherPolicy policy, std::initializer_list<T> set)
    : policy(policy), set(set) { }
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
        const T& value;
        if (!in.Peek(value)) {
            return MatchResult::PENDING;
        }
        Matcher<T>& el = this->p_Elements.at(this->p_Current);
        auto it = std::find(el.set.begin(), el.set.end(), value);
        if (it == el.set.end() && el.policy == MatcherPolicy::MANDATORY) {
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

inline constexpr ByteSpecPolicy M = ByteSpecPolicy::MANDATORY;
inline constexpr ByteSpecPolicy O = ByteSpecPolicy::OPTIONAL;

inline const MatcherSequence<uint8_t> UTF({
    { MatcherPolicy::MANDATORY, { 0x00, 0x01, 0x02, 0x03 } }
});

inline const MatcherSequence<std::byte> UFT({
    { MatcherPolicy::OPTIONAL, Test::BS(0x00, 0x01, 0x02, 0x03) },
    { MatcherPolicy::MANDATORY, Test::BS(0x00, 0x01, 0x02, 0x03) }
});

inline const Pattern UTF16BEBOM({
    { M, 0xFE },
    { M, 0xFF }
});

inline const Pattern UTF16LEBOM({
    { M, 0xFF },
    { M, 0xFE }
});

inline const Pattern UTF8BOM({
    { M, 0xEF },
    { M, 0xBB },
    { M, 0xBF }
});

inline const Pattern UTF16BEXML({
    { M, 0x00 },
    { M, 0x3C },
    { M, 0x00 },
    { M, 0x3F },
    { M, 0x00 },
    { M, 0x78 }
});

inline const Pattern UTF16LEXML({
    { M, 0x3C },
    { M, 0x00 },
    { M, 0x3F },
    { M, 0x00 },
    { M, 0x78 },
    { M, 0x00 }
});

inline const Pattern OPENCOMMENT({
    { M, 0x3C },
    { M, 0x21 },
    { M, 0x2D },
    { M, 0x2D }
});

inline const Pattern CLOSECOMMENT({
    { M, 0x2D },
    { M, 0x2D },
    { M, 0x3E }
});

inline const Pattern OPENMETA({
    { M, 0x3C },
    { M, 0x4D, 0x6D },
    { M, 0x45, 0x65 },
    { M, 0x54, 0x74 },
    { M, 0x41, 0x61 },
    { M, 0x09, 0x0A, 0x0C, 0x0D, 0x20, 0x2F }
});

#define CRAWLER_UPPER_CASE_LETTERS                                                \
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, \
    0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A  \

#define CRAWLER_LOWER_CASE_LETTERS                                                \
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, \
    0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A

#define CRAWLER_LETTERS \
    CRAWLER_UPPER_CASE_LETTERS, CRAWLER_LOWER_CASE_LETTERS

inline const Pattern THREEC({
    { M, 0x3C }, 
    { O, 0x2F },
    { M, CRAWLER_LETTERS }
});

inline const Pattern ESQ({
    { M, 0x3C },
    { M, 0x21, 0x2F, 0x3F }
});

inline const Pattern CLOSETAG({
    { M, 0x3E }
});

inline const Pattern SKIPSEQUENCE({
    { M, 0x09, 0x0A, 0x0C, 0x0D, 0x20, 0x3E }
});

inline const Pattern GETATTRSKIP({
    { M, 0x09, 0x0A, 0x0C, 0x0D, 0x20, 0x2F }
});

inline const Pattern GETATTREQ({
    { M, 0x3D }
});

inline const Pattern GETATTRIFSPACES({
    { M, 0x09, 0x0A, 0x0C, 0x0D, 0x20 }
});

inline const Pattern GETATTRBYTEABORT({
    { M, 0x2F, 0x3E }
});

inline const Pattern GETATTRMAIUSCLETTERS({
    { M, CRAWLER_UPPER_CASE_LETTERS }
});

inline const Pattern GETATTRQUOTE({
    { M, 0x22, 0x27 }
});

inline const Pattern XMLOPENTAG({
    { M, 0x3C },
    { M, 0x3F },
    { M, 0x78 },
    { M, 0x6D },
    { M, 0x6C }
});

inline const Pattern XMLENCODING({
    { M, 0x65 },
    { M, 0x6E },
    { M, 0x63 },
    { M, 0x6F },
    { M, 0x64 },
    { M, 0x69 },
    { M, 0x6E },
    { M, 0x67 }
});

inline const Pattern XMLG({
    { M, 0x67 }
});

#define CRAWLER_SPACE_CONTROL                                                                       \
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, \
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, \
    0x20

inline const Pattern XMLSPACEORCONTROL({
    { M, CRAWLER_SPACE_CONTROL }
});

inline const Pattern SPACESANDSEMICOLON({
    { M, 0x09, 0x0A, 0x0C, 0x0D, 0x20, 0x3B }
});

}

}