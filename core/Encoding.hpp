#pragma once

#include <cstdint>
#include <cstddef>
#include <set>
#include <utility>
#include <initializer_list>
#include <vector>
#include <map>
#include <string>
#include <utility>

namespace Crawler
{

enum class Confidence
{
    TENTATIVE,
    CERTAIN,
    IRRELEVANT,
    UNDEFINED
};

enum class Encoding
{
    UTF8,
    ISO88592,
    ISO88597,
    ISO88598,
    WINDOWS874,
    WINDOWS1250,
    WINDOWS1251,
    WINDOWS1252,
    WINDOWS1254,
    WINDOWS1255,
    WINDOWS1256,
    WINDOWS1257,
    WINDOWS1258,
    GBK,
    BIG5,
    ISO2022JP,
    SHIFTJIS,
    EUCKR,
    UTF16BE,
    UTF16LE,
    XUSERDEFINED,
    UNDEFINED
};

enum class ByteSpecPolicy
{
    MANDATORY,
    OPTIONAL
};

class ByteSpec
{
private:
    const ByteSpecPolicy p_Policy;
    std::set<uint8_t> p_Set;

public:
    ByteSpec(uint8_t exact, ByteSpecPolicy policy);
    ByteSpec(std::initializer_list<uint8_t> set, ByteSpecPolicy policy);
    ByteSpec(uint8_t b1, uint8_t b2, ByteSpecPolicy policy);

    ByteSpecPolicy GetPolicy() const;
    const std::set<uint8_t>& GetSet() const;
};

class Pattern
{
private:
    std::vector<ByteSpec> p_ByteSpecs;

public:
    Pattern(std::initializer_list<ByteSpec> specs);

    bool Match(const uint8_t* const data, size_t count) const;
    size_t Length() const;
};

struct EncodingDetectionResult
{
    Encoding encoding;
    Confidence confidence;
};

enum class GetAttrState
{
    START,
    PROCESSBYTE,
    SPACES,
    VALUE,
    QUOTELOOP,
    FINALPROCESS
};

std::pair<std::string, std::string> GetAnAttribute(const uint8_t* const data, size_t count, size_t* pos);

}