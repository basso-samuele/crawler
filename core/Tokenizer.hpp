#pragma once
#include <string>
#include <variant>
#include <algorithm>

#include "Pipeline.hpp"
#include "Definitions.hpp"

namespace Crawler
{

enum class TokenType
{
    OPENTAG,
    TAGNAME,
    ATTRNAME,
    ATTRVALUE,
    CLOSETAG,
    SELFCLOSETAG,
    TEXTCONTENT,
    UNKNOWN
};

using TokenValue = std::u32string;

class Token
{
private:
    TokenType p_Type;
    TokenValue p_Value;

public:
    Token(TokenType type, TokenValue value);
    Token(TokenType type);
    Token();

    TokenType GetType();
    TokenValue GetValue();
};

template <typename T>
struct WhitespaceSkipState
{
    T parent;
};

struct InitState
{
    std::u32string text;
};

struct TagNameState
{
    std::u32string name;
};

struct AttributeNameState
{
    std::u32string name;
};

struct AttributeValueState
{
    std::u32string value;
};

struct SelfCloseTagState
{
};

struct TextContentState
{
    std::u32string content;
};

struct ExpectEqualSignState
{
};

struct EndOrNewAttributeState
{
};

struct SwitchTagTypeState
{
};

using TokenizerState = std::variant<
    WhitespaceSkipState<InitState>,
    WhitespaceSkipState<TagNameState>,
    WhitespaceSkipState<AttributeNameState>,
    WhitespaceSkipState<AttributeValueState>,
    WhitespaceSkipState<SelfCloseTagState>,
    WhitespaceSkipState<ExpectEqualSignState>,
    WhitespaceSkipState<EndOrNewAttributeState>,
    WhitespaceSkipState<SwitchTagTypeState>,
    InitState,
    TagNameState,
    AttributeNameState,
    AttributeValueState,
    SelfCloseTagState,
    ExpectEqualSignState,
    EndOrNewAttributeState,
    SwitchTagTypeState
>;

template <typename T>
TokenizerState Transition(WhitespaceSkipState<T>& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return WhitespaceSkipState<T>{};

    if (!IsWhitespace(c)) {
        in.Reset();
        return state.parent;
    }

    in.Drop();
    return WhitespaceSkipState<T>{};
}

TokenizerState Transition(InitState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);
TokenizerState Transition(SwitchTagTypeState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);
TokenizerState Transition(TagNameState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);
TokenizerState Transition(AttributeNameState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);
TokenizerState Transition(ExpectEqualSignState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);
TokenizerState Transition(AttributeValueState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);
TokenizerState Transition(SelfCloseTagState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);
TokenizerState Transition(EndOrNewAttributeState& state, Stream<char32_t>& in, TransactionalStream<Token>& out);

class Tokenizer : public Stage<char32_t, Token>
{
private:
    TokenizerState p_State;

public:
    Tokenizer(Stream<char32_t>& in, TransactionalStream<Token>& out);
    void Process() override;
};

}