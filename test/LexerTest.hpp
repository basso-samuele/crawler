#pragma once

#include <vector>

#include "Test.hpp"

#include <parser.h>
#include <lexer.h>
#include <token.h>

namespace Lexer
{

/* Common initialization code. */
#define CRAWLER_LEXER_SETUP(input)                  \
    std::string _input(input);                      \
    CrawlerBuffer _buffer;                          \
    _buffer.base = (unsigned char*)(_input.data()); \
    _buffer.size = _input.length();                 \
    _buffer.eof = true;                             \
    CrawlerParserContext _parser;                   \
    crawler_parser_init(&_parser);                  \
    crawler_parser_bind_buffer(&_parser, &_buffer);


void SingleInputChar() {
    CRAWLER_LEXER_SETUP("a");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_CHARACTER, (int)_parser.current_token.type);
    CRAWLER_ASSERT_EQ((int)_input[0], _parser.current_token.data.str.data[0]);
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void MultipleInputChar() {
    CRAWLER_LEXER_SETUP("abc ");
    for (int i = 0; i < _input.length(); i++) {
        CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
        CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_CHARACTER, (int)_parser.current_token.type);
        CRAWLER_ASSERT_EQ((int)_input[i], _parser.current_token.data.str.data[0]);
    }
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void LessThanSign() {
    CRAWLER_LEXER_SETUP("<");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_ERROR, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void SingleOpenTag() {
    CRAWLER_LEXER_SETUP("<div>");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_START_TAG, (int)_parser.current_token.type);
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_START_TAG, (int)_parser.lexer.last_emitted_start_tag.type);
    CRAWLER_ASSERT_TRUE(!memcmp(
        _parser.current_token.data.start_tag.name.data,
        _parser.lexer.last_emitted_start_tag.data.start_tag.name.data,
        _parser.lexer.last_emitted_start_tag.data.start_tag.name.length
    ));
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void SingleCloseTag() {
    CRAWLER_LEXER_SETUP("</div>");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_END_TAG, (int)_parser.current_token.type);
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void CharTagChar() {
    CRAWLER_LEXER_SETUP("a</div>c");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_CHARACTER, (int)_parser.current_token.type);
    CRAWLER_ASSERT_EQ((int)'a', _parser.current_token.data.str.data[0]);
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_END_TAG, (int)_parser.current_token.type);
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_CHARACTER, (int)_parser.current_token.type);
    CRAWLER_ASSERT_EQ((int)'c', _parser.current_token.data.str.data[0]);
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void IncompleteStartTag() {
    CRAWLER_LEXER_SETUP("<a");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_ERROR, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void IncompleteEndTag() {
    CRAWLER_LEXER_SETUP("</a");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_ERROR, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void EmptyEndTag() {
    CRAWLER_LEXER_SETUP("</>");
    CRAWLER_ASSERT_EQ((int)CRAWLER_LEXER_SUCCESS, (int)crawler_lexer_gen_token(&_parser));
    CRAWLER_ASSERT_EQ((int)CRAWLER_TOKEN_EOF, (int)_parser.current_token.type);
}

void Test() {
    SingleInputChar();
    MultipleInputChar();
    LessThanSign();
    SingleOpenTag();
    SingleCloseTag();
    CharTagChar();
    IncompleteStartTag();
    IncompleteEndTag();
    EmptyEndTag();
}

}