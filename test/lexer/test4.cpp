#include "LexerData.hpp"

#include <string>

namespace Lexer
{

using namespace std::string_literals;
const std::vector<TokenizerTest> kTest4Tests = {
    #include "test4.in"
};

}