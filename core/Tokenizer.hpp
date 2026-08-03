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
    TEXTCONTENT
};

using TokenValue = std::u32string;

class Token
{
private:
    TokenType p_Type;
    TokenValue p_Value;

public:
    Token(TokenType type, TokenValue value)
        : p_Type(type), p_Value(value) {
    }

    Token(TokenType type)
        : p_Type(type) {
    }

    Token() = default;
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

using TokenizerState = std::variant<
    WhitespaceSkipState<InitState>,
    WhitespaceSkipState<TagNameState>,
    WhitespaceSkipState<AttributeNameState>,
    WhitespaceSkipState<AttributeValueState>,
    WhitespaceSkipState<SelfCloseTagState>,
    WhitespaceSkipState<ExpectEqualSignState>,
    WhitespaceSkipState<EndOrNewAttributeState>,
    InitState,
    TagNameState,
    AttributeNameState,
    AttributeValueState,
    SelfCloseTagState,
    ExpectEqualSignState,
    EndOrNewAttributeState
>;

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

TokenizerState Transition(InitState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return InitState{};

    if (IsOpenTag(c)) {
        in.Drop();
        if (!state.text.empty()) {
            out.Put(Token(TokenType::TEXTCONTENT, state.text));
            state.text = std::u32string();
        }
    } else {
        in.Drop();
        state.text.push_back(c);
        return state;
    }
}

TokenizerState Transition(IfOpenTag& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return InitState{};

    if (IsForwardSlash(c)) {
        out.Put(Token(TokenType::CLOSETAG));
        return WhitespaceSkipState<TagNameState>{};
    } else {

    }
}

TokenizerState Transition(TagNameState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return TagNameState{};

    if (IsAlphanumericOrExclamationMark(c)) {
        state.name.push_back(c);
        in.Drop();
        return state;
    } else {
        if (state.name.empty())
        throw "Empty tag name";
        out.Put(Token(TokenType::TAGNAME, state.name));
        in.Reset();
        return WhitespaceSkipState<AttributeNameState>{};
    }
}

TokenizerState Transition(AttributeNameState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return AttributeNameState{};

    in.Drop();
    if (IsAlphanumeric(c)) {
        state.name.push_back(c);
        return state;
    } else if (IsEqualSign(c)) {
        if (state.name.empty()) throw "Empty attribute name";
        out.Put(Token(TokenType::ATTRNAME, state.name));
        return WhitespaceSkipState<AttributeValueState>{};
    } else if (IsCloseBracket(c)) {
        if (!state.name.empty()) out.Put(Token(TokenType::ATTRNAME, state.name));
        out.Put(Token(TokenType::CLOSETAG));
        return WhitespaceSkipState<InitState>{};
    } else if (IsForwardSlash(c)) {
        return SelfCloseTagState{};
    } else if (IsWhitespace(c)) {
        return WhitespaceSkipState<ExpectEqualSignState>{};
    } else {
        throw "Error AttributeNameState";
    }
}

TokenizerState Transition(ExpectEqualSignState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return ExpectEqualSignState{};

    if (IsEqualSign(c)) {
        in.Drop();
        return WhitespaceSkipState<AttributeValueState>{};
    } else {
        throw "Expected equal sign";
    }
}

TokenizerState Transition(AttributeValueState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return AttributeValueState{};

    if (IsAlphanumeric(c)) {
        state.value.push_back(c);
        in.Drop();
        return state;
    } else {
        out.Put(Token(TokenType::ATTRVALUE, state.value));
        in.Reset();
        return WhitespaceSkipState<EndOrNewAttributeState>{};
    }
}

TokenizerState Transition(SelfCloseTagState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return SelfCloseTagState{};

    if (IsCloseBracket(c)) {
        out.Put(Token(TokenType::SELFCLOSETAG));
        in.Drop();
        return WhitespaceSkipState<InitState>{};
    } else {
        throw "Error SelfCloseTagState";
    }
}

TokenizerState Transition(EndOrNewAttributeState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return EndOrNewAttributeState{};

    if (IsAlphanumeric(c)) {
        in.Reset();
        return WhitespaceSkipState<AttributeNameState>{};
    } else if (IsCloseBracket(c)) {
        out.Put(Token(TokenType::CLOSETAG));
        in.Drop();
        return WhitespaceSkipState<InitState>{};
    } else if (IsForwardSlash(c)) {
        in.Drop();
        return SelfCloseTagState{};
    } else {
        throw "Error EndOrNewAttributeState :: "+c;
    }
}

class Tokenizer : public Stage<char32_t, Token>
{
private:
    TokenizerState p_State;

public:
    Tokenizer(Stream<char32_t>& in, TransactionalStream<Token>& out)
    : Stage<char32_t, Token>(in, out), p_State(WhitespaceSkipState<InitState>{}) {}

    void Process() override {
        for (char32_t c; this->p_In.Peek(&c);) {
            this->p_In.Reset();
            this->p_State = std::visit(
                [&](auto& current) {
                    return Transition(current, this->p_In, this->p_Out);
                }, this->p_State);
        }
    }
};

}