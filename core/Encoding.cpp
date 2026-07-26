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

ByteSpecPolicy ByteSpec::GetPolicy() const {
    return this->p_Policy;
}

const std::set<std::byte>& ByteSpec::GetSet() const {
    return this->p_Set;
}

Pattern::Pattern(std::initializer_list<ByteSpec> specs)
: p_ByteSpecs(specs) { }

bool Pattern::Match(const std::byte* const data, size_t count) const {
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

std::pair<std::string, std::string> GetAnAttribute(const std::byte* const data, size_t count, size_t* const pos) {
    GetAttrState state = GetAttrState::START;
    std::string attributeName, attributeValue;
    assert(attributeName.empty());
    assert(attributeValue.empty());

    std::byte quoteLoopB;

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
                char c = static_cast<char>(data[*pos]);
                attributeName += (c + 0x20);
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
                char c = static_cast<char>(data[*pos]);
                attributeValue += (c + 0x20);
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
                    char c = static_cast<char>(data[*pos]);
                    attributeValue += (c + 0x20);
                } else {
                    attributeValue += static_cast<char>(data[*pos]);
                }
            }
            break;

        case GetAttrState::FINALPROCESS:
            if (Patterns::SKIPSEQUENCE.Match(data + *pos, count - *pos)) {
                return { attributeName, attributeValue };
            } else if (Patterns::GETATTRMAIUSCLETTERS.Match(data + *pos, count - *pos)) {
                char c = static_cast<char>(data[*pos]);
                attributeValue += (c + 0x20);
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

void SkipUntil(const std::byte* const data, size_t count, size_t* const pos, const Pattern& pattern) {
    while (((*pos) < count) && !pattern.Match(data + *pos, count - *pos)) {
        (*pos)++;
    }
}

void SkipWhile(const std::byte* const data, size_t count, size_t* const pos, const Pattern& pattern) {
    while (((*pos) < count) && pattern.Match(data + *pos, count - *pos)) {
        (*pos)++;
    }
}

Encoding GetAnXMLEncoding(const std::byte* const data, size_t count, size_t* const pos) {
    size_t encodingPosition = *pos;
    size_t xmlDeclarationEnd = *pos;

    size_t encodingEndPosition;

    std::byte quoteMark;

    if (!Patterns::XMLOPENTAG.Match(data + encodingPosition, count - encodingPosition)) {
        return Encoding::UNDEFINED;
    }

    /* Look for the first 0x3E '>' in the stream. If there is no such byte, return failure. */
    SkipUntil(data, count, &xmlDeclarationEnd, Patterns::CLOSETAG);
    if (!Patterns::CLOSETAG.Match(data + xmlDeclarationEnd, count - xmlDeclarationEnd)) {
        return Encoding::UNDEFINED;
    }

    /* Look for the 'encoding' subsequence in the declaration. */
    SkipUntil(data, xmlDeclarationEnd, &encodingPosition, Patterns::XMLENCODING);
    if (!Patterns::XMLENCODING.Match(data + encodingPosition, count - encodingPosition)) {
        return Encoding::UNDEFINED;
    }

    SkipUntil(data, xmlDeclarationEnd, &encodingPosition, Patterns::XMLG);
    encodingPosition++;
    assert(encodingPosition < xmlDeclarationEnd);

    SkipWhile(data, count, &encodingPosition, Patterns::XMLSPACEORCONTROL);

    if (!Patterns::GETATTREQ.Match(data + encodingPosition, count - encodingPosition)) {
        return Encoding::UNDEFINED;
    }
    encodingPosition++;
    /* Here the encodingPosition is just after '='. */

    SkipWhile(data, count, &encodingPosition, Patterns::XMLSPACEORCONTROL);

    if (!Patterns::GETATTRQUOTE.Match(data + encodingPosition, 1)) {
        return Encoding::UNDEFINED;
    }
    quoteMark = data[encodingPosition];
    encodingPosition++;

    const Pattern quote({
        { ByteSpecPolicy::MANDATORY, quoteMark }
    });

    encodingEndPosition = encodingPosition;
    SkipUntil(data, xmlDeclarationEnd, &encodingEndPosition, quote);
    if (!quote.Match(data + encodingEndPosition, count - encodingEndPosition)) {
        return Encoding::UNDEFINED;
    }

    bool hasSpace = std::any_of(
        data + encodingPosition,
        data + encodingEndPosition,
        [](std::byte b) { return Patterns::XMLSPACEORCONTROL.Match(&b, 1); }
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

        SkipWhile(reinterpret_cast<const std::byte*>(value.data()), value.length(), &pos, Patterns::GETATTRIFSPACES);

        if ((pos < value.length()) && value[pos] != '=') {
            continue;
        }

        pos++;

        SkipWhile(reinterpret_cast<const std::byte*>(value.data()), value.length(), &pos, Patterns::GETATTRIFSPACES);
        
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
            SkipUntil(reinterpret_cast<const std::byte*>(value.data()), value.length(), &endPos, Patterns::SPACESANDSEMICOLON);
            std::string encodingString = value.substr(pos, endPos - pos);
            return EncodingLabelLookup(encodingString);
        }
    }
}

#define CRAWLER_HTTPEQUIV "http-equiv"
#define CRAWLER_CONTENTTYPE "content-type"
#define CRAWLER_CONTENT "content"
#define CRAWLER_CHARSET "charset"

Encoding ParseAttributesAndValues(const std::byte* const data, size_t count, size_t* const pos) {
    std::set<std::string> attributes;
    bool gotPragma = false;

    bool needPragmaValue = false;
    bool* needPragma = nullptr;

    Encoding charsetValue = Encoding::UNDEFINED;
    Encoding* charset = nullptr;

    if (!((*pos) < count)) {
        return Encoding::UNDEFINED;
    }
    std::pair<std::string, std::string> attVal = GetAnAttribute(data, count, pos);
    while (!attVal.first.empty()) {
        if (attributes.find(attVal.first) == attributes.end()) {
            attributes.insert(attVal.first);
            if (!attVal.first.compare(CRAWLER_HTTPEQUIV)) {
                gotPragma = !attVal.second.compare(CRAWLER_CONTENTTYPE);
            } else if (!attVal.first.compare(CRAWLER_CONTENT)) {
                if (charset == nullptr) {
                    Encoding enc = ExtractEncodingFromMetaElement(attVal.second);
                    if (enc != Encoding::UNDEFINED) {
                        charsetValue = enc;
                        charset = &charsetValue;

                        needPragmaValue = true;
                        needPragma = &needPragmaValue;
                    }
                }
            } else if (!attVal.first.compare(CRAWLER_CHARSET)) {
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

    return Encoding::UNDEFINED;
}


Encoding Prescan(const std::byte* const data, size_t count, size_t* const pos) {
    size_t originalPos = *pos;

    if (Patterns::UTF16LEXML.Match(data + *pos, count - *pos)) {
        return Encoding::UTF16LE;
    }
    if (Patterns::UTF16BEXML.Match(data + *pos, count - *pos)) {
        return Encoding::UTF16BE;
    }

    while ((*pos) < count) {
        if (Patterns::OPENCOMMENT.Match(data + *pos, count - *pos)) {
            SkipUntil(data, count, pos, Patterns::CLOSECOMMENT);
        } else if (Patterns::OPENMETA.Match(data + *pos, count - *pos)) {
            SkipUntil(data, count, pos, Patterns::GETATTRSKIP);
            Encoding enc = ParseAttributesAndValues(data, count, pos);
            if (enc != Encoding::UNDEFINED) {
                return enc;
            }
        } else if (Patterns::THREEC.Match(data + *pos, count - *pos)) {
            SkipUntil(data, count, pos, Patterns::SKIPSEQUENCE);
            while ((*pos) < count) {
                std::string first = GetAnAttribute(data, count, pos).first;
                if (first.empty()) break;
            }
        } else if (Patterns::ESQ.Match(data + *pos, count - *pos)) {
            SkipUntil(data, count, pos, Patterns::CLOSETAG);
        }

        (*pos)++;
    }

    *pos = originalPos;
    return GetAnXMLEncoding(data, count, pos);
}

}