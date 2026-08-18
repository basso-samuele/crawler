#include "LexerData.hpp"

namespace Lexer
{

const std::vector<TokenizerTest> kNamedEntityTests = {
    #include "namedEntities.in"
};

}