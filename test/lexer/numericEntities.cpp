#include "LexerData.hpp"

namespace Lexer
{

const std::vector<TokenizerTest> kNumericEntitiesTests = {
    #include "numericEntities.in"
};

}