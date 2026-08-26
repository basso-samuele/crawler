#include "LexerData.hpp"

namespace Lexer
{

using namespace std::string_literals;
const std::vector<TokenizerTest> kEscapeFlagTests = {
    #include "escapeFlag.in"
};

}