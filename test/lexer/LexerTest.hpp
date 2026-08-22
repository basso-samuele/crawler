#pragma once

#include "gtest/gtest.h"
#include "LexerData.hpp"

#include <vector>
#include <variant>

#include <parser.h>
#include <lexer.h>
#include <utils.h>

namespace Lexer
{

CrawlerTokenType tokenType(const ExpectedToken& token) {
    return std::visit([](const auto& t) -> CrawlerTokenType {
        using T = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<T, CharacterToken>)
            return CRAWLER_TOKEN_CHARACTER;
        else if constexpr (std::is_same_v<T, StartTagToken>)
            return CRAWLER_TOKEN_START_TAG;
        else if constexpr (std::is_same_v<T, EndTagToken>)
            return CRAWLER_TOKEN_END_TAG;
        else if constexpr (std::is_same_v<T, CommentToken>)
            return CRAWLER_TOKEN_COMMENT;
        else if constexpr (std::is_same_v<T, DoctypeToken>)
            return CRAWLER_TOKEN_DOCTYPE;
        else if constexpr (std::is_same_v<T, ProcessingInstructionToken>)
            return CRAWLER_TOKEN_PROCESSING_INSTRUCTION;
        else if constexpr (std::is_same_v<T, EOFToken>)
            return CRAWLER_TOKEN_EOF;
        else
            return CRAWLER_TOKEN_TYPE_UNKNOWN;
    }, token);
}

void Equals(const CharacterToken& expected, const CrawlerToken& actual) {
    ASSERT_EQ(expected.data.size(), actual.data.str.length);
    size_t len = std::min(expected.data.size(), actual.data.str.length)*sizeof *expected.data.c_str();
    ASSERT_TRUE(memcmp(expected.data.c_str(), actual.data.str.data, len) == 0);
}

void Equals(const StartTagToken& expected, const CrawlerToken& actual) {
    // Name.
    size_t len = std::min(expected.name.size(), actual.data.start_tag.name.length)*sizeof *expected.name.c_str();
    ASSERT_TRUE(memcmp(expected.name.c_str(), actual.data.start_tag.name.data, len) == 0);
    // Self closing.
    ASSERT_EQ(expected.selfClosing, actual.data.start_tag.is_self_closing);
    // Expected attributes.
    CrawlerAttributeNode* curr = actual.data.start_tag.attributes;
    for (const Attribute& attribute: expected.attributes) {
        // Check if attribute exists.
        ASSERT_TRUE(curr != NULL);
        // Check name equals.
        size_t len = std::min(attribute.name.size(), curr->attribute.name.length)*sizeof *attribute.name.c_str();
        ASSERT_TRUE(memcmp(attribute.name.c_str(), curr->attribute.name.data, len) == 0);
        // Check value equals.
        len = std::min(attribute.value.size(), curr->attribute.value.length)*sizeof *attribute.value.c_str();
        ASSERT_TRUE(memcmp(attribute.value.c_str(), curr->attribute.value.data, len) == 0);
        // Go to next node.
        curr = curr->next;
    }
}

void Equals(const EndTagToken& expected, const CrawlerToken& actual) {
    size_t len = std::min(expected.name.size(), actual.data.end_tag.length)*sizeof *expected.name.c_str();
    ASSERT_TRUE(memcmp(expected.name.c_str(), actual.data.end_tag.data, len) == 0);
}

void Equals(const CommentToken& expected, const CrawlerToken& actual) {
    size_t len = std::min(expected.data.size(), actual.data.str.length)*sizeof *expected.data.c_str();
    ASSERT_TRUE(memcmp(expected.data.c_str(), actual.data.str.data, len) == 0);
}

void Equals(const DoctypeToken& expected, const CrawlerToken& actual) {
    // Name.
    if (expected.name.has_value()) {
        size_t len = std::min(expected.name.value().size(), actual.data.doc_type.name.length)*sizeof *expected.name.value().c_str();
        ASSERT_TRUE(memcmp(expected.name.value().c_str(), actual.data.doc_type.name.data, len) == 0);
    } else {
        ASSERT_EQ(actual.data.doc_type.name.length, 0);
    }
    // Check if public/system id presence corresponds.
    ASSERT_EQ(expected.publicIdentifier.has_value(), actual.data.doc_type.has_public_identifier);
    ASSERT_EQ(expected.systemIdentifier.has_value(), actual.data.doc_type.has_system_identifier);
    if (expected.publicIdentifier.has_value()) {
        const auto& publicIdentifier = expected.publicIdentifier.value();
        size_t len = std::min(publicIdentifier.size(), actual.data.doc_type.public_identifier.length)*sizeof *publicIdentifier.c_str();
        ASSERT_TRUE(memcmp(publicIdentifier.c_str(), actual.data.doc_type.public_identifier.data, len) == 0);
    } else {
        // Verify that has_public_identifier is coherent with the public_identifier's value.
        ASSERT_EQ(actual.data.doc_type.public_identifier.length, 0);
    }
    if (expected.systemIdentifier.has_value()) {
        const auto& systemIdentifier = expected.systemIdentifier.value();
        size_t len = std::min(systemIdentifier.size(), actual.data.doc_type.system_identifier.length)*sizeof *systemIdentifier.c_str();
        ASSERT_TRUE(memcmp(systemIdentifier.c_str(), actual.data.doc_type.system_identifier.data, len) == 0);
    } else {
        // Verify that has_system_identifier is coherent with the system_identifier's value.
        ASSERT_EQ(actual.data.doc_type.system_identifier.length, 0);
    }
    ASSERT_EQ(!expected.correctness, actual.data.doc_type.force_quirks);
}

void Equals(const ProcessingInstructionToken& expected, const CrawlerToken& actual) {
    size_t len = std::min(expected.data.size(), actual.data.proc_in.data.length)*sizeof *expected.data.c_str();
    ASSERT_TRUE(memcmp(expected.data.c_str(), actual.data.proc_in.data.data, len) == 0);

    len = std::min(expected.target.size(), actual.data.proc_in.target.length)*sizeof *expected.data.c_str();
    ASSERT_TRUE(memcmp(expected.target.c_str(), actual.data.proc_in.target.data, len) == 0);
}

void Equals(const EOFToken& expected, const CrawlerToken& actual) {
    // What is that with a neck and no head, two arms and no hands?
}

class Entity : public testing::TestWithParam<TokenizerTest>
{
protected:
    void SetUp() {
        parser.lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        parser.lexer.start_tag_emitted = false;
        parser.lexer.current_attribute_node = NULL;
        crawler_string_create(&parser.lexer.temporary_buffer, 4);
        crawler_string_create(&parser.lexer.last_emitted_start_tag_name, 4);
    }

    static void SetUpTestSuite() {
        crawler_parser_init(&parser);
        crawler_lexer_create(&parser.lexer);
    }

    static void TearDownTestSuite() {
        crawler_lexer_destroy(&parser.lexer);
    }

    inline static CrawlerParserContext parser;
};

TEST_P(Entity, Compare) {
    const auto& tc = GetParam();
    SCOPED_TRACE(tc.description);

    CrawlerBuffer buffer;
    buffer.base = (unsigned char*)tc.input.c_str();
    buffer.size = tc.input.length();
    buffer.eof = true;
    crawler_parser_bind_buffer(&parser, &buffer);

    parser.lexer.current_state = static_cast<CrawlerLexerState>(
        static_cast<std::underlying_type_t<LexerState>>(tc.initialState)
    );

    for (auto& expectedToken : tc.expectedTokens) {
        ASSERT_EQ(CRAWLER_LEXER_SUCCESS, crawler_lexer_gen_token(&parser));
        const CrawlerToken& ct = parser.current_token;

        ASSERT_EQ(tokenType(expectedToken), ct.type);
        std::visit([&](auto& current) {
            Equals(current, ct);
        }, expectedToken);

        crawler_token_destroy(&parser.current_token);
    }
}

#if 1
INSTANTIATE_TEST_SUITE_P(
    NamedEntities,
    Entity,
    ::testing::ValuesIn(
        kNamedEntitiesTests
    )
);

INSTANTIATE_TEST_SUITE_P(
    NumericEntities,
    Entity,
    ::testing::ValuesIn(
        kNumericEntitiesTests
    )
);

INSTANTIATE_TEST_SUITE_P(
    UnicodeChar,
    Entity,
    ::testing::ValuesIn(
        kUnicodeCharTests
    )
);

INSTANTIATE_TEST_SUITE_P(
    Test1,
    Entity,
    ::testing::ValuesIn(
        kTest1Tests
    )
);

INSTANTIATE_TEST_SUITE_P(
    Test2,
    Entity,
    ::testing::ValuesIn(
        kTest2Tests
    )
);

INSTANTIATE_TEST_SUITE_P(
    Test3,
    Entity,
    ::testing::ValuesIn(
        kTest3Tests
    )
);
#endif

INSTANTIATE_TEST_SUITE_P(
    Test4,
    Entity,
    ::testing::ValuesIn(
        kTest4Tests
    )
);

}