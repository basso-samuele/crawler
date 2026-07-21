#pragma once

#include <string>
#include <unordered_map>
#include <algorithm>

#include "Encoding.hpp"

namespace Crawler
{

inline const std::unordered_map<std::string, Encoding> ENCODINGLABELS = {
    {"unicode-1-1-utf-8", Encoding::UTF8},
    {"unicode11utf8", Encoding::UTF8},
    {"unicode20utf8", Encoding::UTF8},
    {"utf-8", Encoding::UTF8},
    {"utf8", Encoding::UTF8},
    {"x-unicode20utf8", Encoding::UTF8},

    {"csisolatin2", Encoding::ISO88592},
    {"iso-8859-2", Encoding::ISO88592},
    {"iso-ir-101", Encoding::ISO88592},
    {"iso8859-2", Encoding::ISO88592},
    {"iso88592", Encoding::ISO88592},
    {"iso_8859-2", Encoding::ISO88592},
    {"iso_8859-2:1987", Encoding::ISO88592},
    {"l2", Encoding::ISO88592},
    {"latin2", Encoding::ISO88592},

    {"csisolatingreek", Encoding::ISO88597},
    {"ecma-118", Encoding::ISO88597},
    {"elot_928", Encoding::ISO88597},
    {"greek", Encoding::ISO88597},
    {"greek8", Encoding::ISO88597},
    {"iso-8859-7", Encoding::ISO88597},
    {"iso-ir-126", Encoding::ISO88597},
    {"iso8859-7", Encoding::ISO88597},
    {"iso88597", Encoding::ISO88597},
    {"iso_8859-7", Encoding::ISO88597},
    {"iso_8859-7:1987", Encoding::ISO88597},
    {"sun_eu_greek", Encoding::ISO88597},

    {"csiso88598e", Encoding::ISO88598},
    {"csisolatinhebrew", Encoding::ISO88598},
    {"hebrew", Encoding::ISO88598},
    {"iso-8859-8", Encoding::ISO88598},
    {"iso-8859-8-e", Encoding::ISO88598},
    {"iso-ir-138", Encoding::ISO88598},
    {"iso8859-8", Encoding::ISO88598},
    {"iso88598", Encoding::ISO88598},
    {"iso_8859-8", Encoding::ISO88598},
    {"iso_8859-8:1988", Encoding::ISO88598},
    {"visual", Encoding::ISO88598},

    {"dos-874", Encoding::WINDOWS874},
    {"iso-8859-11", Encoding::WINDOWS874},
    {"iso8859-11", Encoding::WINDOWS874},
    {"iso885911", Encoding::WINDOWS874},
    {"tis-620", Encoding::WINDOWS874},
    {"windows-874", Encoding::WINDOWS874},

    {"cp1250", Encoding::WINDOWS1250},
    {"windows-1250", Encoding::WINDOWS1250},
    {"x-cp1250", Encoding::WINDOWS1250},

    {"cp1251", Encoding::WINDOWS1251},
    {"windows-1251", Encoding::WINDOWS1251},
    {"x-cp1251", Encoding::WINDOWS1251},

    {"ansi_x3.4-1968", Encoding::WINDOWS1252},
    {"ascii", Encoding::WINDOWS1252},
    {"cp1252", Encoding::WINDOWS1252},
    {"cp819", Encoding::WINDOWS1252},
    {"csisolatin1", Encoding::WINDOWS1252},
    {"ibm819", Encoding::WINDOWS1252},
    {"iso-8859-1", Encoding::WINDOWS1252},
    {"iso-ir-100", Encoding::WINDOWS1252},
    {"iso8859-1", Encoding::WINDOWS1252},
    {"iso88591", Encoding::WINDOWS1252},
    {"iso_8859-1", Encoding::WINDOWS1252},
    {"iso_8859-1:1987", Encoding::WINDOWS1252},
    {"l1", Encoding::WINDOWS1252},
    {"latin1", Encoding::WINDOWS1252},
    {"us-ascii", Encoding::WINDOWS1252},
    {"windows-1252", Encoding::WINDOWS1252},
    {"x-cp1252", Encoding::WINDOWS1252},

    {"cp1254", Encoding::WINDOWS1254},
    {"csisolatin5", Encoding::WINDOWS1254},
    {"iso-8859-9", Encoding::WINDOWS1254},
    {"iso-ir-148", Encoding::WINDOWS1254},
    {"iso8859-9", Encoding::WINDOWS1254},
    {"iso88599", Encoding::WINDOWS1254},
    {"iso_8859-9", Encoding::WINDOWS1254},
    {"iso_8859-9:1989", Encoding::WINDOWS1254},
    {"l5", Encoding::WINDOWS1254},
    {"latin5", Encoding::WINDOWS1254},
    {"windows-1254", Encoding::WINDOWS1254},
    {"x-cp1254", Encoding::WINDOWS1254},

    {"cp1255", Encoding::WINDOWS1255},
    {"windows-1255", Encoding::WINDOWS1255},
    {"x-cp1255", Encoding::WINDOWS1255},

    {"cp1256", Encoding::WINDOWS1256},
    {"windows-1256", Encoding::WINDOWS1256},
    {"x-cp1256", Encoding::WINDOWS1256},

    {"cp1257", Encoding::WINDOWS1257},
    {"windows-1257", Encoding::WINDOWS1257},
    {"x-cp1257", Encoding::WINDOWS1257},

    {"cp1258", Encoding::WINDOWS1258},
    {"windows-1258", Encoding::WINDOWS1258},
    {"x-cp1258", Encoding::WINDOWS1258},

    {"chinese", Encoding::GBK},
    {"csgb2312", Encoding::GBK},
    {"csiso58gb231280", Encoding::GBK},
    {"gb2312", Encoding::GBK},
    {"gb_2312", Encoding::GBK},
    {"gb_2312-80", Encoding::GBK},
    {"gbk", Encoding::GBK},
    {"iso-ir-58", Encoding::GBK},
    {"x-gbk", Encoding::GBK},

    {"big5", Encoding::BIG5},
    {"big5-hkscs", Encoding::BIG5},
    {"cn-big5", Encoding::BIG5},
    {"csbig5", Encoding::BIG5},
    {"x-x-big5", Encoding::BIG5},

    {"csiso2022jp", Encoding::ISO2022JP},
    {"iso-2022-jp", Encoding::ISO2022JP},

    {"csshiftjis", Encoding::SHIFTJIS},
    {"ms932", Encoding::SHIFTJIS},
    {"ms_kanji", Encoding::SHIFTJIS},
    {"shift-jis", Encoding::SHIFTJIS},
    {"shift_jis", Encoding::SHIFTJIS},
    {"sjis", Encoding::SHIFTJIS},
    {"windows-31j", Encoding::SHIFTJIS},
    {"x-sjis", Encoding::SHIFTJIS},

    {"cseuckr", Encoding::EUCKR},
    {"csksc56011987", Encoding::EUCKR},
    {"euc-kr", Encoding::EUCKR},
    {"iso-ir-149", Encoding::EUCKR},
    {"korean", Encoding::EUCKR},
    {"ks_c_5601-1987", Encoding::EUCKR},
    {"ks_c_5601-1989", Encoding::EUCKR},
    {"ksc5601", Encoding::EUCKR},
    {"ksc_5601", Encoding::EUCKR},
    {"windows-949", Encoding::EUCKR},

    {"unicodefffe", Encoding::UTF16BE},
    {"utf-16be", Encoding::UTF16BE},

    {"csunicode", Encoding::UTF16LE},
    {"iso-10646-ucs-2", Encoding::UTF16LE},
    {"ucs-2", Encoding::UTF16LE},
    {"unicode", Encoding::UTF16LE},
    {"unicodefeff", Encoding::UTF16LE},
    {"utf-16", Encoding::UTF16LE},
    {"utf-16le", Encoding::UTF16LE},

    {"x-user-defined", Encoding::XUSERDEFINED}
};

inline Encoding EncodingLabelLookup(std::string& label) {
    std::transform(label.begin(), label.end(), label.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto it = ENCODINGLABELS.find(label);
    if (it != ENCODINGLABELS.end()) {
        return it->second;
    } else {
        return Encoding::UNDEFINED;
    }
}

}