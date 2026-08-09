#include "Definitions.hpp"
#include <algorithm>

namespace Crawler
{

bool IsWhitespace(char32_t c) {
    auto it = std::find(CRAWLER_WHITESPACES.data(), CRAWLER_WHITESPACES.data() + CRAWLER_WHITESPACES.size(), c);
    return it != (CRAWLER_WHITESPACES.data() + CRAWLER_WHITESPACES.size());
}

bool IsAlphanumeric(char32_t c) {
    auto it = std::find(CRAWLER_UNICODE_ALPHANUMERICAL.data(), CRAWLER_UNICODE_ALPHANUMERICAL.data() + CRAWLER_UNICODE_ALPHANUMERICAL.size(), c);
    return it != (CRAWLER_UNICODE_ALPHANUMERICAL.data() + CRAWLER_UNICODE_ALPHANUMERICAL.size());
}

bool IsAlphanumericOrExclamationMark(char32_t c) {
    auto it = std::find(CRAWLER_UNICODE_ALPHANUMERICAL_EM.data(), CRAWLER_UNICODE_ALPHANUMERICAL_EM.data() + CRAWLER_UNICODE_ALPHANUMERICAL_EM.size(), c);
    return it != (CRAWLER_UNICODE_ALPHANUMERICAL_EM.data() + CRAWLER_UNICODE_ALPHANUMERICAL_EM.size());
}

bool IsOpenTag(char32_t c) { return c == char32_t(0x003C); }
bool IsEqualSign(char32_t c) { return c == char32_t(0x003D); }
bool IsCloseBracket(char32_t c) { return c == char32_t(0x003E); }
bool IsForwardSlash(char32_t c) { return c == char32_t(0x002F); }

}