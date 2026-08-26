#include "LexerData.hpp"

#include <string>

namespace Lexer
{

using namespace std::string_literals;
const std::vector<TokenizerTest> kTest3Tests = {
    #include "test3.in"
};

}