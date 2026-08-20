#include "LexerData.hpp"

namespace Lexer
{

const std::vector<TokenizerTest> kUnicodeCharTests = {
    #include "unicodeChars.in"
};

}