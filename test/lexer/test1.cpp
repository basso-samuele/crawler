#include "LexerData.hpp"

namespace Lexer
{

using namespace std::string_literals;
const std::vector<TokenizerTest> kTest1Tests = {
    #include "test1.in"
};

}