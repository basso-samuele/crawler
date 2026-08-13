#include "lexer.h"
#include "parser.h"
#include "stream.h"
#include "vector.h"
#include "attributes.h"
#include "memory.h"
#include "utils.h"

#include <stdbool.h>
#include <assert.h>

static void crawler_lexer_create_character_token(struct CrawlerInternalParserContext* parser) {
    crawler_string_create(&parser->current_token.data.str, 1);
    parser->current_token.type = CRAWLER_TOKEN_CHARACTER;
}

static void crawler_lexer_emit_character(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_string_append(&parser->current_token.data.str, cp);
}

static void crawler_lexer_emit_current_character(struct CrawlerInternalParserContext* parser) {
    crawler_string_append(&parser->current_token.data.str, parser->is.current_code_point);
}

static void crawler_lexer_emit_temporary_buffer_as_character(struct CrawlerInternalParserContext* parser) {
    crawler_string_append_string_buffer(&parser->current_token.data.str, &parser->lexer.temporary_buffer);
}

static void crawler_lexer_create_start_tag_token(struct CrawlerInternalParserContext* parser) {
    parser->current_token.type = CRAWLER_TOKEN_START_TAG;
    crawler_string_create(&parser->current_token.data.start_tag.name, 8);
    crawler_vector_init(&parser->current_token.data.start_tag.attributes);
}

static void crawler_lexer_create_end_tag_token(struct CrawlerInternalParserContext* parser) {
    parser->current_token.type = CRAWLER_TOKEN_END_TAG;
    crawler_string_create(&parser->current_token.data.end_tag, 8);
}

static void crawler_lexer_create_comment_token(struct CrawlerInternalParserContext* parser) {
    parser->current_token.type = CRAWLER_TOKEN_COMMENT;
    crawler_string_create(&parser->current_token.data.str, 16);
}

static void crawler_lexer_temporary_to_empty_string(struct CrawlerInternalParserContext* parser) {
    // Resets the content of temporary buffer withour memory management concerns.
    parser->lexer.temporary_buffer.length = 0;
}

static void crawler_lexer_start_new_attribute(struct CrawlerInternalParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    CrawlerAttribute* attribute = (CrawlerAttribute*)_crawler_alloc(sizeof(CrawlerAttribute));
    crawler_attribute_init(attribute);
    crawler_attribute_create(attribute);
    crawler_vector_append(&parser->current_token.data.start_tag.attributes, (void*)attribute);
}

static void crawler_lexer_append_to_current_attr_name(struct CrawlerInternalParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    size_t current_attribute_pos = parser->current_token.data.start_tag.attributes.size-1;
    assert(current_attribute_pos >= 1);
    CrawlerAttribute* current_attribute = parser->current_token.data.start_tag.attributes.data[current_attribute_pos];
    crawler_string_append(&current_attribute->name, cp);
}

static void crawler_lexer_append_to_current_attr_value(struct CrawlerInternalParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    size_t current_attribute_pos = parser->current_token.data.start_tag.attributes.size-1;
    assert(current_attribute_pos >= 1);
    CrawlerAttribute* current_attribute = parser->current_token.data.start_tag.attributes.data[current_attribute_pos];
    crawler_string_append(&current_attribute->value, cp);
}

static bool crawler_lexer_is_appropriate_end_tag_token(struct CrawlerInternalParserContext* parser) {
    // An appropriate end tag token is an end tag token whose tag name matches
    // the tag name of the last start tag to have been emitted from this tokenizer,
    // if any. If no start tag has been emitted from this tokenizer, then no end tag token is appropriate.
    assert(parser->current_token.type == CRAWLER_TOKEN_END_TAG);
    return crawler_string_compare(&parser->current_token.data.end_tag, &parser->lexer.last_emitted_start_tag.data.start_tag.name);
}

static void crawler_lexer_check_current_attribute_unique(struct CrawlerInternalParserContext* parser) {
    crawler_debug("Missing duplicate attribute check.");
}

static void crawler_lexer_emit_current_tag_token(struct CrawlerInternalParserContext* parser) {
    switch(parser->current_token.type) {
        case CRAWLER_TOKEN_START_TAG:
            crawler_token_clone(&parser->lexer.last_emitted_start_tag, &parser->current_token);
            break;
        case CRAWLER_TOKEN_END_TAG:
        case CRAWLER_TOKEN_TAG:
            // No need for extra steps.
            break;
        default:
            // Should never happen.
            assert(false);
    }
}

static void crawler_lexer_append_tag_name(struct CrawlerInternalParserContext* parser, int cp) {
    switch (parser->current_token.type) {
    case CRAWLER_TOKEN_DOCTYPE:
        crawler_string_append(&parser->current_token.data.doc_type.name, cp);
        break;
    case CRAWLER_TOKEN_START_TAG:
        crawler_string_append(&parser->current_token.data.start_tag.name, cp);
        break;
    case CRAWLER_TOKEN_END_TAG:
        crawler_string_append(&parser->current_token.data.end_tag, cp);
        break;
    default:
        // The only cases where appending to the tag name is meaningful are listed above.
        assert(false);
    }
}

static bool crawler_lexer_is_ascii_upper_alpha(int cp) {
    return ((cp >= 0x0041) && (cp <= 0x005A));
}

static bool crawler_lexer_is_ascii_lower_alpha(int cp) {
    return ((cp >= 0x0061) && (cp <= 0x007A));
}

static bool crawler_lexer_is_ascii_alpha(int cp) {
    return ((cp >= 0x0041) && (cp <= 0x005A)) ||
           ((cp >= 0x0061) && (cp <= 0x007A));
}

static CrawlerLexerResult handle_data_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0026: // AMPERSAND
        parser->lexer.return_state = CRAWLER_LEXER_STATE_DATA;
        parser->lexer.current_state = CRAWLER_LEXER_STATE_CHARACTER_REFERENCE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003C: // LESS-THAN-SIGN
        parser->lexer.current_state = CRAWLER_LEXER_STATE_TAG_OPEN;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

static CrawlerLexerResult handle_rcdata_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0026: // AMPERSAND
        parser->lexer.return_state = CRAWLER_LEXER_STATE_RCDATA;
        parser->lexer.current_state = CRAWLER_LEXER_STATE_CHARACTER_REFERENCE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003C: // LESS-THAN-SIGN
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RCDATA_LESS_THAN_SIGN;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        // Preparing current_token.data.str to receive data.
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

static CrawlerLexerResult handle_rawtext_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x003C: // LESS-THAN-SIGN
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RAWTEXT_LESS_THAN_SIGN;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

static CrawlerLexerResult handle_script_data_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x003C: // LESS-THAN-SIGN
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_LESS_THAN_SIGN;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

static CrawlerLexerResult handle_plaintext_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

static CrawlerLexerResult handle_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0021: // EXCLAMATION MARK (!)
        parser->lexer.current_state = CRAWLER_LEXER_STATE_MARKUP_DECLARATION_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
        parser->lexer.current_state = CRAWLER_LEXER_STATE_END_TAG_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003F: // QUESTION MARK (?)
        crawler_lexer_temporary_to_empty_string(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_PROCESSING_INSTRUCTION_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_BEFORE_TAG_NAME, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        // Return to state data will emit an EOF token if presented with a -1 codepoint. Reconsuming.
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        if (crawler_lexer_is_ascii_alpha(cp)) { // ASCII alpha
            // Create a new start tag token, set its tag name to the empty string.
            crawler_lexer_create_start_tag_token(parser);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_TAG_NAME;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else { // Anything else
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME, cp, parser->is.current_total_offset);
            crawler_lexer_create_character_token(parser);
            crawler_lexer_emit_character(parser, 0x003C);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_ERROR;
        }
    }
}

static CrawlerLexerResult handle_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_END_TAG_NAME, cp, parser->is.current_total_offset);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_BEFORE_TAG_NAME, cp, parser->is.current_total_offset);
        // Emit a U+003C LESS-THAN SIGN character token, a U+002F SOLIDUS character token.
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        crawler_lexer_emit_character(parser, 0x002F);
        // Return to state data will emit an EOF token if presented with a -1 codepoint. Reconsuming.
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        if (crawler_lexer_is_ascii_alpha(cp)) { // ASCII alpha
            // Create a new end tag token, set its tag name to the empty string.
            crawler_lexer_create_end_tag_token(parser);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_TAG_NAME;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else { // Anything else
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME, cp, parser->is.current_total_offset);
            // Create a comment token whose data is the empty string.
            crawler_lexer_create_comment_token(parser);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_BOGUS_COMMENT;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    }
}

static CrawlerLexerResult handle_tag_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_lexer_emit_current_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_append_tag_name(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        if (crawler_lexer_is_ascii_upper_alpha(cp)) { // ASCII upper alpha
            crawler_lexer_append_tag_name(parser, cp + 0x0020);
        } else { // Anything else
            crawler_lexer_append_tag_name(parser, cp);
        }
        return CRAWLER_LEXER_NEXT_CP;
    }
}

static CrawlerLexerResult handle_rcdata_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        crawler_lexer_temporary_to_empty_string(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RCDATA_END_TAG_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RCDATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

static CrawlerLexerResult handle_rcdata_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_reset(parser);
    if (crawler_lexer_is_ascii_alpha(cp)) { // ASCII alpha
        crawler_lexer_create_end_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RCDATA_END_TAG_NAME;
        return CRAWLER_LEXER_NEXT_CP;
    } else { // Anything else
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        crawler_lexer_emit_character(parser, 0x002F);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RCDATA;
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#rcdata-end-tag-name-state
static CrawlerLexerResult handle_rcdata_end_tag_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            crawler_lexer_emit_current_tag_token(parser);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        }
        break;
    default:
        // Possible future improvement: the append method may lowercase each code point.
        if (crawler_lexer_is_ascii_upper_alpha(cp)) { // ASCII upper alpha
            crawler_lexer_append_tag_name(parser, cp+0x0020);
            crawler_string_append(&parser->lexer.temporary_buffer, cp+0x0020);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else if (crawler_lexer_is_ascii_lower_alpha(cp)) { // ASCII lower alpha
            crawler_lexer_append_tag_name(parser, cp);
            crawler_string_append(&parser->lexer.temporary_buffer, cp);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    }
    // Anything else
    crawler_lexer_create_character_token(parser);
    crawler_lexer_emit_character(parser, 0x003C);
    crawler_lexer_emit_character(parser, 0x002F);
    crawler_lexer_emit_temporary_buffer_as_character(parser);
    parser->lexer.current_state = CRAWLER_LEXER_STATE_RCDATA;
    crawler_stream_reset(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#rawtext-less-than-sign-state
static CrawlerLexerResult handle_rawtext_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        crawler_lexer_temporary_to_empty_string(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RAWTEXT_END_TAG_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RAWTEXT;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#rawtext-end-tag-open-state
static CrawlerLexerResult handle_rawtext_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_reset(parser);
    if (crawler_lexer_is_ascii_alpha(cp)) {
        crawler_lexer_create_end_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RAWTEXT_END_TAG_NAME;
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        crawler_lexer_emit_character(parser, 0x002F);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_RAWTEXT;
        return CRAWLER_LEXER_SUCCESS;
    }
}


// https://html.spec.whatwg.org/#rawtext-end-tag-name-state
static CrawlerLexerResult handle_rawtext_end_tag_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            crawler_lexer_emit_current_tag_token(parser);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        }
        break;
    default:
        // Possible future improvement: the append method may lowercase each code point.
        if (crawler_lexer_is_ascii_upper_alpha(cp)) { // ASCII upper alpha
            crawler_lexer_append_tag_name(parser, cp+0x0020);
            crawler_string_append(&parser->lexer.temporary_buffer, cp+0x0020);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else if (crawler_lexer_is_ascii_lower_alpha(cp)) { // ASCII lower alpha
            crawler_lexer_append_tag_name(parser, cp);
            crawler_string_append(&parser->lexer.temporary_buffer, cp);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    }
    // Anything else
    crawler_lexer_create_character_token(parser);
    crawler_lexer_emit_character(parser, 0x003C);
    crawler_lexer_emit_character(parser, 0x002F);
    crawler_lexer_emit_temporary_buffer_as_character(parser);
    parser->lexer.current_state = CRAWLER_LEXER_STATE_RAWTEXT;
    crawler_stream_reset(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#script-data-less-than-sign-state
static CrawlerLexerResult handle_script_data_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        crawler_lexer_temporary_to_empty_string(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_END_TAG_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0021: // EXCLAMATION MARK (!)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        crawler_lexer_emit_character(parser, 0x0021);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPE_START;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-end-tag-open-state
static CrawlerLexerResult handle_script_data_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_reset(parser);
    if (crawler_lexer_is_ascii_alpha(cp)) {
        crawler_lexer_create_end_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_END_TAG_NAME;
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        crawler_lexer_emit_character(parser, 0x002F);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA;
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-end-tag-name-state
static CrawlerLexerResult handle_script_data_end_tag_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            crawler_lexer_emit_current_tag_token(parser);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        }
        break;
    default:
        // Possible future improvement: the append method may lowercase each code point.
        if (crawler_lexer_is_ascii_upper_alpha(cp)) { // ASCII upper alpha
            crawler_lexer_append_tag_name(parser, cp+0x0020);
            crawler_string_append(&parser->lexer.temporary_buffer, cp+0x0020);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else if (crawler_lexer_is_ascii_lower_alpha(cp)) { // ASCII lower alpha
            crawler_lexer_append_tag_name(parser, cp);
            crawler_string_append(&parser->lexer.temporary_buffer, cp);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    }
    // Anything else
    crawler_lexer_create_character_token(parser);
    crawler_lexer_emit_character(parser, 0x003C);
    crawler_lexer_emit_character(parser, 0x002F);
    crawler_lexer_emit_temporary_buffer_as_character(parser);
    parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA;
    crawler_stream_reset(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#script-data-escape-start-state
static CrawlerLexerResult handle_script_data_escape_start_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPE_START_DASH;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#script-data-escape-start-dash-state
static CrawlerLexerResult handle_script_data_escape_start_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-state
static CrawlerLexerResult handle_script_data_escaped_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_DASH;
        return CRAWLER_LEXER_SUCCESS;
    case 0x003C: // LESS-THAN SIGN (<)
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-dash-state
static CrawlerLexerResult handle_script_data_escaped_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH;
        return CRAWLER_LEXER_SUCCESS;
    case 0x003C: // LESS-THAN SIGN (<)
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-dash-dash-state
static CrawlerLexerResult handle_script_data_escaped_dash_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        return CRAWLER_LEXER_SUCCESS;
    case 0x003C: // LESS-THAN SIGN (<)
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003E);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA;
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-less-than-sign-state
static CrawlerLexerResult handle_script_data_escaped_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        crawler_lexer_temporary_to_empty_string(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (crawler_lexer_is_ascii_alpha(cp)) {
            crawler_lexer_temporary_to_empty_string(parser);
            crawler_lexer_create_character_token(parser);
            crawler_lexer_emit_character(parser, 0x003C);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_START;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else {
            crawler_lexer_create_character_token(parser);
            crawler_lexer_emit_character(parser, 0x003C);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_SUCCESS;
        }
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-end-tag-open-state
static CrawlerLexerResult handle_script_data_escaped_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_reset(parser);
    if (crawler_lexer_is_ascii_alpha(cp)) {
        crawler_lexer_create_end_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_NAME;
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        crawler_lexer_emit_character(parser, 0x002F);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-end-tag-name-state
static CrawlerLexerResult handle_script_data_escaped_end_tag_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (crawler_lexer_is_appropriate_end_tag_token(parser)) {
            crawler_lexer_emit_current_tag_token(parser);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        }
        break;
    default:
        // Possible future improvement: the append method may lowercase each code point.
        if (crawler_lexer_is_ascii_upper_alpha(cp)) { // ASCII upper alpha
            crawler_lexer_append_tag_name(parser, cp+0x0020);
            crawler_string_append(&parser->lexer.temporary_buffer, cp+0x0020);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else if (crawler_lexer_is_ascii_lower_alpha(cp)) { // ASCII lower alpha
            crawler_lexer_append_tag_name(parser, cp);
            crawler_string_append(&parser->lexer.temporary_buffer, cp);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    }
    // Anything else
    crawler_lexer_create_character_token(parser);
    crawler_lexer_emit_character(parser, 0x003C);
    crawler_lexer_emit_character(parser, 0x002F);
    crawler_lexer_emit_temporary_buffer_as_character(parser);
    parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
    crawler_stream_reset(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#script-data-double-escape-start-state
static CrawlerLexerResult handle_script_data_double_escape_start_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
    case 0x002F: // SOLIDUS (/)
    case 0x003E: // GREATER-THAN SIGN (>)
        if (crawler_string_compare_with_literal(&parser->lexer.temporary_buffer, "script")) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        } else {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        }
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (crawler_lexer_is_ascii_upper_alpha(cp)) {
            crawler_string_append(&parser->lexer.temporary_buffer, cp+0x0020);
            crawler_lexer_create_character_token(parser);
            crawler_lexer_emit_current_character(parser);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else if (crawler_lexer_is_ascii_lower_alpha(cp)) {
            crawler_string_append(&parser->lexer.temporary_buffer, cp);
            crawler_lexer_create_character_token(parser);
            crawler_lexer_emit_current_character(parser);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-state
static CrawlerLexerResult handle_script_data_double_escaped_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH;
        return CRAWLER_LEXER_SUCCESS;
    case 0x003C: // LESS-THAN SIGN (<)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN;
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-dash-state
static CrawlerLexerResult handle_script_data_double_escaped_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH;
        return CRAWLER_LEXER_SUCCESS;
    case 0x003C: // LESS-THAN SIGN (<)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN;
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-dash-dash-state
static CrawlerLexerResult handle_script_data_double_escaped_dash_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002D);
        return CRAWLER_LEXER_SUCCESS;
    case 0x003C: // LESS-THAN SIGN (<)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN;
        return CRAWLER_LEXER_SUCCESS;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x003E);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA;
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-less-than-sign-state
static CrawlerLexerResult handle_script_data_double_escaped_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        crawler_lexer_temporary_to_empty_string(parser);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0x002F);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_END;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#script-data-double-escape-end-state
static CrawlerLexerResult handle_script_data_double_escape_end_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009:
    case 0x000A:
    case 0x000C:
    case 0x0020:
    case 0x002F:
    case 0x003E:
        if (crawler_string_compare_with_literal(&parser->lexer.temporary_buffer, "script")) {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        } else {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        }
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_current_character(parser);
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (crawler_lexer_is_ascii_upper_alpha(cp)) {
            crawler_string_append(&parser->lexer.temporary_buffer, cp+0x0020);
            crawler_lexer_create_character_token(parser);
            crawler_lexer_emit_current_character(parser);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else if (crawler_lexer_is_ascii_lower_alpha(cp)) {
            crawler_string_append(&parser->lexer.temporary_buffer, cp);
            crawler_lexer_create_character_token(parser);
            crawler_lexer_emit_current_character(parser);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else {
            parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    }
}

// https://html.spec.whatwg.org/#before-attribute-name-state
static CrawlerLexerResult handle_before_attribute_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
    case 0x003E: // GREATER-THAN SIGN (>)
    case -1: // EOF
        parser->lexer.current_state = CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_NAME;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003D: // EQUALS SIGN (=)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME, cp, parser->is.current_total_offset);
        crawler_lexer_start_new_attribute(parser);
        crawler_lexer_append_to_current_attr_name(parser, cp);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_ATTRIBUTE_NAME;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        crawler_lexer_start_new_attribute(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_ATTRIBUTE_NAME;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#attribute-name-state
static CrawlerLexerResult handle_attribute_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
    case 0x002F: // SOLIDUS (/)
    case 0x003E: // GREATER-THAN SIGN (>)
    case -1: // EOF
        crawler_lexer_check_current_attribute_unique(parser);
        crawler_stream_reset(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_NAME;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003D: // EQUALS SIGN (=)
        crawler_lexer_check_current_attribute_unique(parser);
        crawler_stream_commit(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_VALUE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_append_to_current_attr_name(parser, 0xFFFD);
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
    case 0x0027: // APOSTROPHE (')
    case 0x003C: // LESS-THAN SIGN (<)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME, cp, parser->is.current_total_offset);
        crawler_lexer_append_to_current_attr_name(parser, cp);
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (crawler_lexer_is_ascii_upper_alpha(cp)) {
            crawler_lexer_append_to_current_attr_name(parser, cp+0x0020);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else {
            crawler_lexer_append_to_current_attr_name(parser, cp);
            crawler_stream_commit(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    }
}

// https://html.spec.whatwg.org/#after-attribute-name-state
static CrawlerLexerResult handle_after_attribute_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009:
    case 0x000A:
    case 0x000C:
    case 0x0020:
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F:
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003D:
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_VALUE;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E:
        crawler_lexer_emit_current_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_start_new_attribute(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_ATTRIBUTE_NAME;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#before-attribute-value-state
static CrawlerLexerResult handle_before_attribute_value_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
        parser->lexer.current_state = CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        parser->lexer.current_state = CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_ATTRIBUTE_VALUE, cp, parser->is.current_total_offset);
        crawler_lexer_emit_current_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        parser->lexer.current_state = CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_UNQUOTED;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#attribute-value-(double-quoted)-state
static CrawlerLexerResult handle_attribute_value_double_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0022: // QUOTATION MARK (")
        parser->lexer.current_state = CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0026: // AMPERSAND (&)
        parser->lexer.return_state = CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED;
        parser->lexer.current_state = CRAWLER_LEXER_STATE_CHARACTER_REFERENCE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_append_to_current_attr_value(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_append_to_current_attr_value(parser, cp);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#attribute-value-(single-quoted)-state
static CrawlerLexerResult handle_attribute_value_single_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0022: // QUOTATION MARK (")
        parser->lexer.current_state = CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0026: // AMPERSAND (&)
        parser->lexer.return_state = CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED;
        parser->lexer.current_state = CRAWLER_LEXER_STATE_CHARACTER_REFERENCE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_append_to_current_attr_value(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_append_to_current_attr_value(parser, cp);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#attribute-value-(unquoted)-state
static CrawlerLexerResult handle_attribute_value_unquoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0026: // AMPERSAND (&)
        parser->lexer.return_state = CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_UNQUOTED;
        parser->lexer.current_state = CRAWLER_LEXER_STATE_CHARACTER_REFERENCE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_lexer_emit_current_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_append_to_current_attr_value(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
    case 0x0027: // APOSTROPHE (')
    case 0x003C: // LESS-THAN SIGN (<)
    case 0x003D: // EQUALS SIGN (=)
    case 0x0060: // GRAVE ACCENT (`)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE, cp, parser->is.current_total_offset);
        crawler_lexer_append_to_current_attr_value(parser, cp);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_lexer_append_to_current_attr_value(parser, cp);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-attribute-value-(quoted)-state
static CrawlerLexerResult handle_after_attribute_value_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_lexer_emit_current_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES, cp, parser->is.current_total_offset);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#self-closing-start-tag-state
static CrawlerLexerResult handle_self_closing_start_tag_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        parser->current_token.data.start_tag.is_self_closing = true;
        crawler_lexer_emit_current_tag_token(parser);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_total_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_SOLIDUS_IN_TAG, cp, parser->is.current_total_offset);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}



typedef CrawlerLexerResult(*CrawlerLexerHandler)(struct CrawlerInternalParserContext*, int);
static CrawlerLexerHandler kCrawlerHandlerDispatchTable[] = {
    handle_data_state,
    handle_rcdata_state,
    handle_rawtext_state,
    handle_script_data_state,
    handle_plaintext_state,
    handle_tag_open_state,
    handle_end_tag_open_state,
    handle_tag_name_state,
    handle_rcdata_less_than_sign_state,
    handle_rcdata_end_tag_open_state,

    handle_rcdata_end_tag_name_state,
    handle_rawtext_less_than_sign_state,
    handle_rawtext_end_tag_open_state,
    handle_rawtext_end_tag_name_state,
    handle_script_data_less_than_sign_state,
    handle_script_data_end_tag_open_state,
    handle_script_data_end_tag_name_state,
    handle_script_data_escape_start_state,
    handle_script_data_escape_start_dash_state,
    handle_script_data_escaped_state,

    handle_script_data_escaped_dash_state,
    handle_script_data_escaped_dash_dash_state,
    handle_script_data_escaped_less_than_sign_state,
    handle_script_data_escaped_end_tag_open_state,
    handle_script_data_escaped_end_tag_name_state,
    handle_script_data_double_escape_start_state,
    handle_script_data_double_escaped_state,
    handle_script_data_double_escaped_dash_state,
    handle_script_data_double_escaped_dash_dash_state,
    handle_script_data_double_escaped_less_than_sign_state,

    handle_script_data_double_escape_end_state,
    handle_before_attribute_name_state,
    handle_attribute_name_state,
    handle_after_attribute_name_state,
    handle_before_attribute_value_state,
    handle_attribute_value_double_quoted_state,
    handle_attribute_value_single_quoted_state,
    handle_attribute_value_unquoted_state,
    handle_after_attribute_value_quoted_state,
    handle_self_closing_start_tag_state
};

void crawler_lexer_init(struct CrawlerInternalParserContext* parser) {
    CrawlerLexerContext* lexer = &parser->lexer;
    // The state machine must start in the data state.
    lexer->current_state = CRAWLER_LEXER_STATE_DATA;
    crawler_string_init(&lexer->temporary_buffer);
    crawler_token_init(&lexer->last_emitted_start_tag);
}

CrawlerLexerResult crawler_lexer_gen_token(struct CrawlerInternalParserContext* parser) {
    CrawlerUTF8Stream* is = &parser->is;
    CrawlerLexerResult step_result;
    do {
        if (crawler_stream_peek(parser) == CRAWLER_STREAM_MISSING_ELEMENT)
            return CRAWLER_LEXER_MISSING_CP;
        step_result = kCrawlerHandlerDispatchTable[parser->lexer.current_state](parser, is->current_code_point);
    } while (step_result == CRAWLER_LEXER_NEXT_CP);
    return step_result;
}