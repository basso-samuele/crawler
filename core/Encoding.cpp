#include "Encoding.hpp"

#include <numeric>

namespace Crawler
{

ByteSpec::ByteSpec(uint8_t exact, ByteSpecPolicy policy)
: p_Policy(policy), p_Set({ exact }) { }

ByteSpec::ByteSpec(std::initializer_list<uint8_t> set, ByteSpecPolicy policy)
: p_Policy(policy), p_Set(set) { }

ByteSpec::ByteSpec(uint8_t b1, uint8_t b2, ByteSpecPolicy policy)
: p_Policy(policy),
  p_Set([&]{
    std::vector<uint8_t> v(b2 - b1 + 1);
    std::iota(v.begin(), v.end(), b1);
    return std::set<uint8_t>(v.begin(), v.end());
  }()) { }

ByteSpecPolicy ByteSpec::GetPolicy() const {
    return this->p_Policy;
}

const std::set<uint8_t>& ByteSpec::GetSet() const {
    return this->p_Set;
}

Pattern::Pattern(std::initializer_list<ByteSpec> specs)
: p_ByteSpecs(specs) { }

bool Pattern::Match(const std::vector<uint8_t>& data, size_t pos) const {
    for (const ByteSpec& spec : this->p_ByteSpecs) {
        bool byteMatchSuccess =
            (spec.GetPolicy() == ByteSpecPolicy::OPTIONAL) ||
            (spec.GetSet().find(data[pos]) != spec.GetSet().end());
        if (!byteMatchSuccess) {
            return false;
        }
        pos++;
    }

    return true;
}

EncodingDetectionResult DetectBOMHeader(const std::vector<uint8_t>& data) {
    const Pattern UTF16BE_PAT({
        { 0xFE, ByteSpecPolicy::MANDATORY },
        { 0xFF, ByteSpecPolicy::MANDATORY }
    });

    if (UTF16BE_PAT.Match(data, 0)) {
        return { Encoding::UTF16BE, Confidence::CERTAIN };
    }

    const Pattern UTF16LE_PAT({
        { 0xFF, ByteSpecPolicy::MANDATORY },
        { 0xFE, ByteSpecPolicy::MANDATORY }
    });

    if (UTF16LE_PAT.Match(data, 0)) {
        return { Encoding::UTF16LE, Confidence::CERTAIN };
    }

    const Pattern UTF8_PAT({
        { 0xEF, ByteSpecPolicy::MANDATORY },
        { 0xBB, ByteSpecPolicy::MANDATORY },
        { 0xBF, ByteSpecPolicy::MANDATORY }
    });

    if (UTF8_PAT.Match(data, 0)) {
        return { Encoding::UTF8, Confidence::CERTAIN };
    }

    return { Encoding::UNDEFINED, Confidence::UNDEFINED };
}

}