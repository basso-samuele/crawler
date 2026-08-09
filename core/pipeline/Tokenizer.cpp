#include "Tokenizer.hpp"

namespace Crawler
{

Token::Token(TokenType type, TokenValue value)
: p_Type(type), p_Value(value) { }

Token::Token(TokenType type)
: p_Type(type) { }

Token::Token()
: p_Type(TokenType::UNKNOWN) { }

TokenType Token::GetType() {
    return this->p_Type;
}

TokenValue Token::GetValue() {
    return this->p_Value;
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
        return WhitespaceSkipState<SwitchTagTypeState>{};
    }
    else {
        in.Drop();
        state.text.push_back(c);
        return state;
    }
}

TokenizerState Transition(SwitchTagTypeState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return InitState{};

    if (IsForwardSlash(c)) {
        in.Drop();
        out.Put(Token(TokenType::CLOSETAG));
        return WhitespaceSkipState<TagNameState>{};
    }
    else if (IsAlphanumericOrExclamationMark(c)) {
        in.Reset();
        out.Put(Token(TokenType::OPENTAG));
        return TagNameState{};
    }
    else {
        throw "Error Switch";
    }
}

TokenizerState Transition(TagNameState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return TagNameState{};

    if (IsAlphanumericOrExclamationMark(c)) {
        state.name.push_back(c);
        in.Drop();
        return state;
    }
    else {
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
    }
    else if (IsEqualSign(c)) {
        if (state.name.empty()) throw "Empty attribute name";
        out.Put(Token(TokenType::ATTRNAME, state.name));
        return WhitespaceSkipState<AttributeValueState>{};
    }
    else if (IsCloseBracket(c)) {
        if (!state.name.empty()) out.Put(Token(TokenType::ATTRNAME, state.name));
        return WhitespaceSkipState<InitState>{};
    }
    else if (IsForwardSlash(c)) {
        return SelfCloseTagState{};
    }
    else if (IsWhitespace(c)) {
        return WhitespaceSkipState<ExpectEqualSignState>{};
    }
    else {
        throw "Error AttributeNameState";
    }
}

TokenizerState Transition(ExpectEqualSignState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return ExpectEqualSignState{};

    if (IsEqualSign(c)) {
        in.Drop();
        return WhitespaceSkipState<AttributeValueState>{};
    }
    else {
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
    }
    else {
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
    }
    else {
        throw "Error SelfCloseTagState";
    }
}

TokenizerState Transition(EndOrNewAttributeState& state, Stream<char32_t>& in, TransactionalStream<Token>& out) {
    char32_t c;
    if (!in.Peek(&c)) return EndOrNewAttributeState{};

    if (IsAlphanumeric(c)) {
        in.Reset();
        return WhitespaceSkipState<AttributeNameState>{};
    }
    else if (IsCloseBracket(c)) {
        in.Drop();
        return WhitespaceSkipState<InitState>{};
    }
    else if (IsForwardSlash(c)) {
        in.Drop();
        return SelfCloseTagState{};
    }
    else {
        throw "Error EndOrNewAttributeState :: " + c;
    }
}

Tokenizer::Tokenizer(Stream<char32_t>& in, TransactionalStream<Token>& out)
: Stage<char32_t, Token>(in, out), p_State(WhitespaceSkipState<InitState>{}) {}

void Tokenizer::Process() {
    for (char32_t c; this->p_In.Peek(&c);) {
        this->p_In.Reset();
        this->p_State = std::visit(
            [&](auto& current) {
                return Transition(current, this->p_In, this->p_Out);
            }, this->p_State);
    }

    if (this->p_In.End()) {
        this->p_Out.SetEndFlag();
    }
}

}