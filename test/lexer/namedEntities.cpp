#include "LexerData.hpp"

namespace Lexer
{

const std::vector<TokenizerTest> kNamedEntitiesTests = {
    #include "namedEntities.in"
};

}