#include "Encoding.hpp"

#include <numeric>
#include <map>
#include <string>
#include <cassert>

#include "Patterns.hpp"

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

bool Pattern::Match(const uint8_t* const data, size_t count) const {
    size_t pos = 0;

    for (const ByteSpec& spec : this->p_ByteSpecs) {
        bool isByteMandatory = spec.GetPolicy() == ByteSpecPolicy::MANDATORY;

        if (!(pos < count)) {
            return false;
        }
        bool byteMatchSuccess = spec.GetSet().find(data[pos]) != spec.GetSet().end();

        if (byteMatchSuccess) {
            pos++;
        } else if (isByteMandatory) {
            return false;
        }
    }

    return true;
}

size_t Pattern::Length() const {
    return this->p_ByteSpecs.size();
}

std::pair<std::string, std::string> GetAnAttribute(const uint8_t* const data, size_t count, size_t* pos) {
    GetAttrState state = GetAttrState::START;
    std::string attributeName, attributeValue;
    assert(attributeName.empty());
    assert(attributeValue.empty());

    uint8_t quoteLoopB;

    while((*pos) < count) {
        switch (state) {

        case GetAttrState::START:
            if (Patterns::GETATTRSKIP.Match(data + *pos, count - *pos)) {
                (*pos)++;
            } else if (Patterns::CLOSETAG.Match(data + *pos, count - *pos)) {
                return { "", "" };
            } else {
                state = GetAttrState::PROCESSBYTE;
            }
            break;

        case GetAttrState::PROCESSBYTE:
            if (Patterns::GETATTREQ.Match(data + *pos, count - *pos) && !attributeName.empty()) {
                (*pos)++;
                state = GetAttrState::VALUE;
            } else if (Patterns::GETATTRIFSPACES.Match(data + *pos, count - *pos)) {
                state = GetAttrState::SPACES;
            } else if (Patterns::GETATTRBYTEABORT.Match(data + *pos, count - *pos)) {
                assert(attributeValue.empty());
                return { attributeName, attributeValue };
            } else if (Patterns::GETATTRMAIUSCLETTERS.Match(data + *pos, count - *pos)) {
                attributeName += static_cast<char>(data[*pos]+0x20);
                (*pos)++;
            } else {
                attributeName += static_cast<char>(data[*pos]);
                (*pos)++;
            }
            break;

        case GetAttrState::SPACES:
            if (Patterns::GETATTRIFSPACES.Match(data + *pos, count - *pos)) {
                (*pos)++;
            } else if (!Patterns::GETATTREQ.Match(data + *pos, count - *pos)) {
                assert(attributeValue.empty());
                return { attributeName, attributeValue };
            } else {
                (*pos)++;
                state = GetAttrState::VALUE;
            }
            break;

        case GetAttrState::VALUE:
            if (Patterns::GETATTRIFSPACES.Match(data + *pos, count - *pos)) {
                (*pos)++;
            } else if (Patterns::GETATTRQUOTE.Match(data + *pos, count - *pos)) {
                quoteLoopB = data[*pos];
                state = GetAttrState::QUOTELOOP;
            } else if (Patterns::CLOSETAG.Match(data + *pos, count - *pos)) {
                assert(attributeValue.empty());
                return { attributeName, attributeValue };
            } else if (Patterns::GETATTRMAIUSCLETTERS.Match(data + *pos, count - *pos)) {
                attributeValue += static_cast<char>(data[*pos]+0x20);
                (*pos)++;
                state = GetAttrState::FINALPROCESS;
            } else {
                attributeValue += static_cast<char>(data[*pos]);
                (*pos)++;
                state = GetAttrState::FINALPROCESS;
            }
            break;

        case GetAttrState::QUOTELOOP:
            (*pos)++;
            if ((*pos) < count) {
                if (data[*pos] == quoteLoopB) {
                    (*pos)++;
                    return { attributeName, attributeValue };
                } else if (Patterns::GETATTRMAIUSCLETTERS.Match(data + *pos, count - *pos)) {
                    attributeValue += static_cast<char>(data[*pos]+0x20);
                } else {
                    attributeValue += static_cast<char>(data[*pos]);
                }
            }
            break;

        case GetAttrState::FINALPROCESS:
            if (Patterns::SKIPSEQUENCE.Match(data + *pos, count - *pos)) {
                return { attributeName, attributeValue };
            } else if (Patterns::GETATTRMAIUSCLETTERS.Match(data + *pos, count - *pos)) {
                attributeValue += static_cast<char>(data[*pos]+0x20);
            } else {
                attributeValue += static_cast<char>(data[*pos]);
            }
            (*pos)++;
            break;

        default:
            break;
        }
    }

    return { attributeName, attributeValue };
}

}