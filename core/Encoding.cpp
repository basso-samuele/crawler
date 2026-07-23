#include "Encoding.hpp"

#include <numeric>
#include <unordered_map>
#include <string>
#include <cassert>
#include <algorithm>
#include <string>

#include "Patterns.hpp"
#include "EncodingLabels.hpp"

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

std::pair<std::string, std::string> GetAnAttribute(const uint8_t* const data, size_t count, size_t* const pos) {
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

Encoding GetAnXMLEncoding(const uint8_t* const data, size_t count, size_t* const pos) {
    size_t encodingPosition = *pos;
    size_t xmlDeclarationEnd = *pos;

    size_t encodingEndPosition;

    uint8_t quoteMark;

    if (!Patterns::XMLOPENTAG.Match(data + encodingPosition, count - encodingPosition)) {
        return Encoding::UNDEFINED;
    }

    /* Look for the first 0x3E '>' in the stream. If there is no such byte, return failure. */
    while ((xmlDeclarationEnd < count) && !Patterns::CLOSETAG.Match(data + xmlDeclarationEnd, count - xmlDeclarationEnd)) {
        xmlDeclarationEnd++;
    }
    if (!Patterns::CLOSETAG.Match(data + xmlDeclarationEnd, count - xmlDeclarationEnd)) {
        return Encoding::UNDEFINED;
    }

    /* Look for the 'encoding' subsequence in the declaration. */
    while ((encodingPosition < xmlDeclarationEnd) && !Patterns::XMLENCODING.Match(data + encodingPosition, count - encodingPosition)) {
        encodingPosition++;
    }
    if (!Patterns::XMLENCODING.Match(data + encodingPosition, count - encodingPosition)) {
        return Encoding::UNDEFINED;
    }

    do {
        encodingPosition++;
    } while (!Patterns::XMLG.Match(data + encodingPosition, count - encodingPosition));
    encodingPosition++;
    assert(encodingPosition < xmlDeclarationEnd);

    while (Patterns::XMLSPACEORCONTROL.Match(data + encodingPosition, count - encodingPosition)) {
        encodingPosition++;
    }

    if (!Patterns::GETATTREQ.Match(data + encodingPosition, count - encodingPosition)) {
        return Encoding::UNDEFINED;
    }
    encodingPosition++;
    /* Here the encodingPosition is just after '='. */

    while (Patterns::XMLSPACEORCONTROL.Match(data + encodingPosition, count - encodingPosition)) {
        encodingPosition++;
    }

    quoteMark = data[encodingPosition];
    /* If I cannot find a quote, then failure. */
    if (!Patterns::GETATTRQUOTE.Match(&quoteMark, 1)) {
        return Encoding::UNDEFINED;
    }
    encodingPosition++;

    const Pattern quote({
        { quoteMark, ByteSpecPolicy::MANDATORY }
    });

    encodingEndPosition = encodingPosition;
    while ((encodingEndPosition < xmlDeclarationEnd) && !quote.Match(data + encodingEndPosition, count - encodingEndPosition)) {
        encodingEndPosition++;
    }
    if (!quote.Match(data + encodingEndPosition, count - encodingEndPosition)) {
        return Encoding::UNDEFINED;
    }

    bool hasSpace = std::any_of(
        data + encodingPosition,
        data + encodingEndPosition,
        [](uint8_t b) { return Patterns::XMLSPACEORCONTROL.Match(&b, 1); }
    );
    if (hasSpace) {
        return Encoding::UNDEFINED;
    }

    std::string potentialEncoding(reinterpret_cast<const char*>(data + encodingPosition), encodingEndPosition - encodingPosition);

    Encoding enc = EncodingLabelLookup(potentialEncoding);
    if (enc == Encoding::UTF16BE || enc == Encoding::UTF16LE) {
        enc = Encoding::UTF8;
    }

    return enc;
}

Encoding ExtractEncodingFromMetaElement(const std::string& value) {
    size_t len = value.length();
    size_t pos = 0;

    while (true) {
        std::string sLower = value;
        std::transform(sLower.begin(), sLower.end(), sLower.begin(), ::tolower);

        const std::string cset("charset");
        size_t charsetPos = sLower.find(cset, pos);
        if (charsetPos == std::string::npos) {
            return Encoding::UNDEFINED;
        }

        // move past "charset"
        pos = charsetPos + cset.length();

        while (pos < value.length() && (value[pos] == ' ' || value[pos] == '\t' || 
            value[pos] == '\n' || value[pos] == '\r' || value[pos] == '\f')) {
            pos++;
        }

        if ((pos < value.length()) && value[pos] != '=') {
            continue;
        }

        pos++;

        while (pos < value.length() && (value[pos] == ' ' || value[pos] == '\t' || 
               value[pos] == '\n' || value[pos] == '\r' || value[pos] == '\f')) {
            pos++;
        }
        
        if (pos >= value.length()) {
            return Encoding::UNDEFINED;
        }

        if (value[pos] == '"') {
            size_t endQuote = value.find('"', pos + 1);
            if (endQuote != std::string::npos) {
                std::string encodingString = value.substr(pos + 1, endQuote - pos - 1);
                return EncodingLabelLookup(encodingString);
            } else {
                return Encoding::UNDEFINED;
            }
        } else if (value[pos] == '\'') {
            size_t endQuote = value.find('\'', pos + 1);
            if (endQuote != std::string::npos) {
                std::string encodingStr = value.substr(pos + 1, endQuote - pos - 1);
                return EncodingLabelLookup(encodingStr);
            } else {
                return Encoding::UNDEFINED;
            }
        } else {
            size_t endPos = pos;
            while (endPos < value.length() && value[endPos] != ' ' && value[endPos] != '\t' && 
                   value[endPos] != '\n' && value[endPos] != '\r' && value[endPos] != '\f' && 
                   value[endPos] != ';') {
                endPos++;
            }
            
            std::string encodingString = value.substr(pos, endPos - pos);
            return EncodingLabelLookup(encodingString);
        }
    }
}

Encoding Prescan(const uint8_t* const data, size_t count, size_t* const pos) {
    size_t originalPos = *pos;

    if (Patterns::UTF16LEXML.Match(data + *pos, count - *pos)) {
        return Encoding::UTF16LE;
    }
    if (Patterns::UTF16BEXML.Match(data + *pos, count - *pos)) {
        return Encoding::UTF16BE;
    }

    while ((*pos) < count) {
        if (Patterns::OPENCOMMENT.Match(data + *pos, count - *pos)) {
            while (((*pos) < count) && !Patterns::CLOSECOMMENT.Match(data + *pos, count - *pos)) {
                (*pos)++;
            }
        } else if (Patterns::OPENMETA.Match(data + *pos, count - *pos)) {
            while (((*pos) < count) && !Patterns::GETATTRSKIP.Match(data + *pos, count - *pos)) {
                (*pos)++;
            }

            // Steps 2 to 5
            std::unordered_map<std::string, std::string> attributes;
            bool gotPragma = false;

            bool needPragmaValue = false;
            bool* needPragma = nullptr;

            Encoding charsetValue = Encoding::UNDEFINED;
            Encoding* charset = nullptr;

            if (!((*pos) < count)) continue;
            std::pair<std::string, std::string> attVal = GetAnAttribute(data, count, pos);
            while (!attVal.first.empty()) {
                if (attributes.find(attVal.first) == attributes.end()) {
                    attributes.insert({ attVal.first, attVal.second });

                    if (!attVal.first.compare("http-equiv")) {
                        gotPragma = !attVal.second.compare("content-type");
                    } else if (!attVal.first.compare("content")) {
                        if (charset == nullptr) {
                            Encoding enc = ExtractEncodingFromMetaElement(attVal.second);
                            if (enc != Encoding::UNDEFINED) {
                                charsetValue = enc;
                                charset = &charsetValue;

                                needPragmaValue = true;
                                needPragma = &needPragmaValue;
                            }
                        }
                    } else if (!attVal.first.compare("charset")) {
                        charsetValue = EncodingLabelLookup(attVal.second);
                        charset = &charsetValue;
                        needPragmaValue = false;
                        needPragma = &needPragmaValue;
                    }
                }
                attVal = GetAnAttribute(data, count, pos);
            }

            if (needPragma != nullptr) {
                if (!(*needPragma && !gotPragma)) {
                    assert(charset != nullptr);
                    if (*charset != Encoding::UNDEFINED) {
                        if (*charset == Encoding::UTF16BE || *charset == Encoding::UTF16LE) {
                            charsetValue = Encoding::UTF8;
                        } else if (*charset == Encoding::XUSERDEFINED) {
                            charsetValue = Encoding::WINDOWS1252;
                        }

                        return charsetValue;
                    }
                }
            }
        } else if (Patterns::THREEC.Match(data + *pos, count - *pos)) {
            while (((*pos) < count) && !Patterns::SKIPSEQUENCE.Match(data + *pos, count - *pos)) {
                (*pos)++;
            }
            while ((*pos) < count) {
                std::string first = GetAnAttribute(data, count, pos).first;
                if (first.empty()) break;
            }
        } else if (Patterns::ESQ.Match(data + *pos, count - *pos)) {
            while (((*pos) < count) && !Patterns::CLOSETAG.Match(data + *pos, count - *pos)) {
                (*pos)++;
            }
        }

        (*pos)++;
    }

    *pos = originalPos;
    return GetAnXMLEncoding(data, count, pos);
}

}