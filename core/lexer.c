#include "lexer.h"
#include "parser.h"
#include "stream.h"
#include "attributes.h"
#include "utils.h"
#include "named_ref.h"

#include <stdbool.h>
#include <assert.h>

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

static bool is_ascii_alphanumeric(int cp) {
    return (cp >= 65 && cp <= 90) ||
           (cp >= 97 && cp <= 122) ||
           (cp >= 48 && cp <= 57);
}

static void switch_state(CrawlerParserContext* parser, CrawlerLexerState state) {
    parser->lexer.current_state = state;
}

static void switch_to_return_state(CrawlerParserContext* parser) {
    parser->lexer.current_state = parser->lexer.return_state;
}

static void stream_reset(CrawlerParserContext* parser) {
    crawler_stream_reset(&parser->is);
}

static void stream_commit(CrawlerParserContext* parser) {
    crawler_stream_commit(&parser->is);
}

static void stream_reconsume(CrawlerParserContext* parser) {
    crawler_stream_reconsume(&parser->is);
}

static void char_ref_code_zero(CrawlerParserContext* parser) {
    parser->lexer.character_reference_code = 0;
}

static bool create_doctype_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_TYPE_UNKNOWN);
    if (!crawler_string_create(&parser->current_token.data.doc_type.name, 8))
        return false;
    if (!crawler_string_create(&parser->current_token.data.doc_type.public_identifier, 8))
        return false;
    if (!crawler_string_create(&parser->current_token.data.doc_type.system_identifier, 8))
        return false;
    parser->current_token.data.doc_type.has_public_identifier = false;
    parser->current_token.data.doc_type.has_system_identifier = false;
    parser->current_token.data.doc_type.force_quirks = false;
    parser->current_token.type = CRAWLER_TOKEN_DOCTYPE;
}

static bool create_start_tag_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_TYPE_UNKNOWN);
    if (!crawler_string_create(&parser->current_token.data.start_tag.name, 8))
        return false;
    parser->current_token.data.start_tag.is_self_closing = false;
    parser->current_token.data.start_tag.attributes = NULL;
    parser->current_token.type = CRAWLER_TOKEN_START_TAG;
}

static bool create_end_tag_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_TYPE_UNKNOWN);
    if (!crawler_string_create(&parser->current_token.data.end_tag, 8))
        return false;
    parser->current_token.type = CRAWLER_TOKEN_END_TAG;
}

static bool create_comment_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_TYPE_UNKNOWN);
    if (!crawler_string_create(&parser->current_token.data.str, 16))
        return false;
    parser->current_token.type = CRAWLER_TOKEN_COMMENT;
}

static bool create_character_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_TYPE_UNKNOWN);
    if (!crawler_string_create(&parser->current_token.data.start_tag.name, 1))
        return false;
    parser->current_token.type = CRAWLER_TOKEN_CHARACTER;
}

static void create_eof_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_TYPE_UNKNOWN);
    parser->current_token.type = CRAWLER_TOKEN_EOF;
}

static void create_processing_instruction_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_TYPE_UNKNOWN);
    assert(false);
}

static bool emit_character(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_CHARACTER);
    return crawler_string_append(&parser->current_token.data.str, cp);
}

static bool emit_current_character(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_CHARACTER);
    return crawler_string_append(&parser->current_token.data.str, parser->is.current_code_point);
}

static void temporary_to_empty_string(CrawlerParserContext* parser) {
    parser->lexer.temporary_buffer.length = 0;
}

static bool temporary_append(CrawlerParserContext* parser, int cp) {
    return crawler_string_append(&parser->lexer.temporary_buffer, cp);
}

static bool emit_temporary_buffer_character(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_CHARACTER);
    if (!crawler_string_append_string_buffer(&parser->current_token.data.str, &parser->lexer.temporary_buffer))
        return false;
    crawler_debug("Emitted temporary buffer as characters, deleting the buffer.");
    temporary_to_empty_string(parser);
}

static bool start_new_attribute(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    assert(!parser->lexer.current_attribute_node);
    parser->lexer.current_attribute_node = _crawler_alloc(sizeof *parser->lexer.current_attribute_node);
    crawler_attribute_node_init(parser->lexer.current_attribute_node);
    return crawler_attribute_node_create(parser->lexer.current_attribute_node);
}

static bool append_to_current_attribute_name(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    assert(parser->lexer.current_attribute_node);
    return crawler_string_append(&parser->lexer.current_attribute_node->attribute.name, cp);
}

static bool append_to_current_attribute_value(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    assert(parser->lexer.current_attribute_node);
    return crawler_string_append(&parser->lexer.current_attribute_node->attribute.value, cp);
}

static void finalize_current_attribute(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    assert(parser->lexer.current_attribute_node);
    crawler_attribute_list_insert(&parser->current_token.data.start_tag.attributes, parser->lexer.current_attribute_node);
    parser->lexer.current_attribute_node = NULL;
}

static void discard_current_attribute(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    assert(parser->lexer.current_attribute_node);
    crawler_attribute_node_destroy(parser->lexer.current_attribute_node);
    _crawler_free(parser->lexer.current_attribute_node);
    parser->lexer.current_attribute_node = NULL;
}

static bool is_appropriate_end_tag_token(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_END_TAG);
    return
        parser->lexer.start_tag_emitted &&
        crawler_string_compare(&parser->current_token.data.end_tag, &parser->lexer.last_emitted_start_tag_name);
}

static bool check_current_attribute_unique(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
    assert(parser->lexer.current_attribute_node);
    CrawlerAttributeNode* it = parser->current_token.data.start_tag.attributes;
    while (it) {
        bool match =
            crawler_string_compare(&it->attribute.name, &parser->lexer.current_attribute_node->attribute.name);
        if (match) return false;
        it = it->next;
    }
    return true;
}

static bool emit_current_tag_token(CrawlerParserContext* parser) {
    switch(parser->current_token.type) {
        case CRAWLER_TOKEN_START_TAG:
            return crawler_string_clone(&parser->lexer.last_emitted_start_tag_name, &parser->current_token.data.start_tag.name);
        case CRAWLER_TOKEN_END_TAG:
            return true;
        default:
            assert(false);
    }
}

static bool append_tag_name(CrawlerParserContext* parser, int cp) {
    switch (parser->current_token.type) {
    case CRAWLER_TOKEN_DOCTYPE:
        return crawler_string_append(&parser->current_token.data.doc_type.name, cp);
    case CRAWLER_TOKEN_START_TAG:
        return crawler_string_append(&parser->current_token.data.start_tag.name, cp);
    case CRAWLER_TOKEN_END_TAG:
        return crawler_string_append(&parser->current_token.data.end_tag, cp);
    default:
        // The only cases where appending to the tag name is meaningful are listed above.
        assert(false);
    }
}

static bool convert_temporary_to_comment(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
    // 0x003F (?)
    if (!crawler_string_append(&parser->current_token.data.str, 0x003F))
        return false;
    if (!crawler_string_append_string_buffer(&parser->current_token.data.str, &parser->lexer.temporary_buffer))
        return false;
    crawler_debug("Converted temporary buffer to comment, deleting the buffer.");
    temporary_to_empty_string(parser);
}

bool consumed_as_part_of_an_attribute(CrawlerParserContext* parser) {
    return
        (parser->lexer.return_state == CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED) ||
        (parser->lexer.return_state == CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED) ||
        (parser->lexer.return_state == CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_UNQUOTED);
}

static bool flush_code_points_consumed_as_a_character_reference(CrawlerParserContext* parser) {
    if (consumed_as_part_of_an_attribute(parser)) {
        if (!crawler_string_append_string_buffer(
            &parser->lexer.current_attribute_node->attribute.value,
            &parser->lexer.temporary_buffer))
            return false;
        crawler_debug("Appended temporary buffer to attribute value, deleting the buffer.");
        temporary_to_empty_string(parser);
    } else {
        if (!create_character_token(parser))
            return false;
        emit_temporary_buffer_character(parser);
    }
}












#if 0

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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_BEFORE_TAG_NAME, cp, parser->is.current_code_point_offset);
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
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_END_TAG_NAME, cp, parser->is.current_code_point_offset);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_BEFORE_TAG_NAME, cp, parser->is.current_code_point_offset);
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
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_append_tag_name(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_create_character_token(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED;
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_append_to_current_attr_name(parser, 0xFFFD);
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
    case 0x0027: // APOSTROPHE (')
    case 0x003C: // LESS-THAN SIGN (<)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_ATTRIBUTE_VALUE, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_append_to_current_attr_value(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_append_to_current_attr_value(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point_offset);
        crawler_lexer_append_to_current_attr_value(parser, 0xFFFD);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
    case 0x0027: // APOSTROPHE (')
    case 0x003C: // LESS-THAN SIGN (<)
    case 0x003D: // EQUALS SIGN (=)
    case 0x0060: // GRAVE ACCENT (`)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE, cp, parser->is.current_code_point_offset);
        crawler_lexer_append_to_current_attr_value(parser, cp);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES, cp, parser->is.current_code_point_offset);
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
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point_offset);
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_SOLIDUS_IN_TAG, cp, parser->is.current_code_point_offset);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}


#endif

// https://html.spec.whatwg.org/#bogus-comment-state
static CrawlerLexerResult handle_bogus_comment_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        emit_current_comment_token(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        emit_current_comment_token(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        append_to_comment(parser, 0xFFFD);
        stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        append_to_comment(parser, cp);
        stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#markup-declaration-open-state
static CrawlerLexerResult handle_markup_declaration_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    CrawlerUTF8Stream* stream = &parser->is;

    if (crawler_stream_consume_match(stream, "--", 2, false)) {
        if (!create_comment_token(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_START);
        return CRAWLER_LEXER_SUCCESS;
    } else if (crawler_stream_consume_match(stream, "DOCTYPE", 7, false)) {
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    } else if (crawler_stream_consume_match(stream, "[CDATA[", 7, false)) {
        // Missing stack of open elements implementation.
        assert(false);
    } else {
        crawler_parser_register_error(parser, CRAWLER_ERROR_INCORRECTLY_OPENED_COMMENT);
        if (!create_comment_token(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#character-reference-state
static CrawlerLexerResult handle_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    temporary_to_empty_string(parser);
    if (!temporary_append(parser, 0x0026)) // AMPERSAND (&)
        return CRAWLER_LEXER_FAILURE;

    switch(cp) {
    case 0x0023: // NUMBER SIGN (#)
        if (!temporary_append(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE);
        stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_alphanumeric(cp)) {
            switch_state(parser, CRAWLER_LEXER_STATE_NAMED_CHARACTER_REFERENCE);
        } else {
            if (!flush_code_points_consumed_as_a_character_reference(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_to_return_state(parser);
        }
        stream_reconsume(parser);
        return CRAWLER_CR_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#named-character-reference-state
static CrawlerLexerResult handle_named_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    if (!temporary_append(parser, cp))
        return CRAWLER_LEXER_FAILURE;

    CrawlerCharacterReference cr;
    CrawlerNamedReferenceResult cr_result = crawler_named_reference_step(parser, cp, &cr);

    switch(cr_result) {
    case CRAWLER_CR_SUCCESS:
        int next_input_character;
        CrawlerStreamResult next_input_character_result =
            crawler_stream_peek(parser, &next_input_character);
        if (next_input_character_result == CRAWLER_STREAM_ERROR)
            return CRAWLER_LEXER_FAILURE;
        if (next_input_character_result == CRAWLER_STREAM_MISSING_ELEMENT) {
            // The current structure forces a character reference reconsumption.
            stream_reset(parser);
            temporary_to_empty_string(parser);
            return CRAWLER_LEXER_MISSING_CP;
        }

        bool historical =
            consumed_as_part_of_an_attribute(parser) &&
            cp != 0x003B && // SEMICOLON (;)
            (next_input_character == 0x003D || is_ascii_alphanumeric(next_input_character)); // EQUALS SIGN (=)
        if (historical) {
            if (!flush_code_points_consumed_as_a_character_reference(parser))
                return CRAWLER_LEXER_FAILURE;
        } else {
            if (cp != 0x003B) // SEMICOLON (;)
                crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            temporary_to_empty_string(parser);
            assert(cr.first != 0 || cr.second != 0);
            if (cr.first != 0)
                if (!crawler_string_append(&parser->lexer.temporary_buffer, cr.first))
                    return CRAWLER_LEXER_FAILURE;
            if (cr.second != 0)
                if (!crawler_string_append(&parser->lexer.temporary_buffer, cr.second))
                    return CRAWLER_LEXER_FAILURE;
        }
        switch_to_return_state(parser);
        return CRAWLER_CR_SUCCESS;
    case CRAWLER_CR_FAILURE:
        if (!flush_code_points_consumed_as_a_character_reference(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_AMBIGUOUS_AMPERSAND);
        return CRAWLER_LEXER_SUCCESS;
    case CRAWLER_CR_NEXT_CP:
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#ambiguous-ampersand-state
static CrawlerLexerResult handle_ambiguous_ampersand_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003B: // SEMICOLON (;)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE);
        stream_reconsume(parser);
        switch_to_return_state(parser);
        return CRAWLER_CR_SUCCESS;
    default:
        if (is_ascii_alphanumeric(cp)) {
            if (consumed_as_part_of_an_attribute(parser)) {
                if (!append_to_current_attribute_value(parser, cp))
                    return CRAWLER_LEXER_FAILURE;
            } else {
                crawler_lexer_create_character_token(parser);
                if (!emit_current_character(parser))
                    return CRAWLER_LEXER_FAILURE;
                stream_commit(parser);
                return CRAWLER_CR_SUCCESS;
            }
        } else {
            stream_reconsume(parser);
            switch_to_return_state(parser);
            return CRAWLER_CR_SUCCESS;
        }
    }
}

// https://html.spec.whatwg.org/#numeric-character-reference-state
static CrawlerLexerResult handle_numeric_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    char_ref_code_zero(parser);
    switch(cp) {
    case 0x0078: // LATIN SMALL LETTER X
    case 0x0058: // LATIN CAPITAL LETTER X
        if (!temporary_append(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_HEXADECIMAL_CHARACTER_REFERENCE_START);
        stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_digit(cp)) {
            switch_state(parser, CRAWLER_LEXER_STATE_DECIMAL_CHARACTER_REFERENCE);
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
            if (!flush_code_points_consumed_as_a_character_reference(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_to_return_state(parser);
        }
        stream_reconsume(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#hexadecimal-character-reference-start-state
static CrawlerLexerResult handle_hexadecimal_character_reference_start_state(struct CrawlerInternalParserContext* parser, int cp) {
    if (is_ascii_hex_digit(cp)) {
        switch_state(parser, CRAWLER_LEXER_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
    } else {
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
        if (!flush_code_points_consumed_as_a_character_reference(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_to_return_state(parser);
    }
    stream_reconsume(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#hexadecimal-character-reference-state
static CrawlerLexerResult handle_hexadecimal_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003B: // SEMICOLON (;)
        switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
        stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_digit(cp)) {
            parser->lexer.character_reference_code *= 16;
            parser->lexer.character_reference_code += cp - 0x0030;
            stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else if (is_ascii_upper_hex_digit(cp)) {
            parser->lexer.character_reference_code *= 16;
            parser->lexer.character_reference_code += cp - 0x0037;
            stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else if (is_ascii_lower_hex_digit(cp)) {
            parser->lexer.character_reference_code *= 16;
            parser->lexer.character_reference_code += cp - 0x0057;
            stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
            stream_reconsume(parser);
            return CRAWLER_LEXER_SUCCESS;
        }
    }
}

// https://html.spec.whatwg.org/#decimal-character-reference-state
static CrawlerLexerResult handle_decimal_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003B: // SEMICOLON (;)
        switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
        stream_commit(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_digit(cp)) {
            parser->lexer.character_reference_code *= 10;
            parser->lexer.character_reference_code += cp - 0x0030;
            stream_commit(parser);
            return CRAWLER_LEXER_SUCCESS;
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
            stream_reconsume(parser);
            return CRAWLER_LEXER_SUCCESS;
        }
    }
}

static const int windows_1252[0x20] = {
    /* 0x80 */ 0x20AC,
    /* 0x81 */ -1,
    /* 0x82 */ 0x201A,
    /* 0x83 */ 0x0192,
    /* 0x84 */ 0x201E,
    /* 0x85 */ 0x2026,
    /* 0x86 */ 0x2020,
    /* 0x87 */ 0x2021,
    /* 0x88 */ 0x02C6,
    /* 0x89 */ 0x2030,
    /* 0x8A */ 0x0160,
    /* 0x8B */ 0x2039,
    /* 0x8C */ 0x0152,
    /* 0x8D */ -1,
    /* 0x8E */ 0x017D,
    /* 0x8F */ -1,
    /* 0x90 */ -1,
    /* 0x91 */ 0x2018,
    /* 0x92 */ 0x2019,
    /* 0x93 */ 0x201C,
    /* 0x94 */ 0x201D,
    /* 0x95 */ 0x2022,
    /* 0x96 */ 0x2013,
    /* 0x97 */ 0x2014,
    /* 0x98 */ 0x02DC,
    /* 0x99 */ 0x2122,
    /* 0x9A */ 0x0161,
    /* 0x9B */ 0x203A,
    /* 0x9C */ 0x0153,
    /* 0x9D */ -1,
    /* 0x9E */ 0x017E,
    /* 0x9F */ 0x0178,
};

// https://html.spec.whatwg.org/#numeric-character-reference-end-state
static CrawlerLexerResult handle_numeric_character_reference_end_state(struct CrawlerInternalParserContext* parser, int cp) {
    // The state does not require character consumption.
    stream_reconsume(parser);

    int crc = parser->lexer.character_reference_code;
    if (crc == 0x00) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_NULL_CHARACTER_REFERENCE);
        crc = 0xFFFD;
    } else if (crc > 0x10FFFF) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_CHARACTER_REFERENCE_OUTSIDE_UNICODE_RANGE);
        crc = 0xFFFD;
    } else if (is_surrogate(crc)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_SURROGATE_CHARACTER_REFERENCE);
        crc = 0xFFFD;
    } else if (is_noncharacter(crc)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_NONCHARACTER_CHARACTER_REFERENCE);
    } else if (crc == 0x0D || (is_control(crc) && !is_ascii_whitespace(crc))) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_CONTROL_CHARACTER_REFERENCE);
    } else if (crc >= 0x80 && crc <= 0x9F) {
        if (windows_1252[crc-0x80] != -1)
            crc = windows_1252[crc-0x80];
    }

    temporary_to_empty_string(parser);
    temporary_append(parser, crc);
    flush_code_points_consumed_as_a_character_reference(parser);
    switch_to_return_state(parser);
    return CRAWLER_LEXER_SUCCESS;
}

typedef CrawlerLexerResult(*CrawlerLexerHandler)(struct CrawlerInternalParserContext*, int);
static CrawlerLexerHandler kCrawlerHandlerDispatchTable[] = {
#if 0
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
    handle_self_closing_start_tag_state,

    handle_bogus_comment_state,
    handle_markup_declaration_open_state,

    // 77
    handle_character_reference_state,
    handle_named_character_reference_state,
    handle_ambiguous_ampersand_state,
    handle_numeric_character_reference_state,
    handle_hexadecimal_character_reference_start_state,
    handle_hexadecimal_character_reference_state,
    handle_decimal_character_reference_state,
    handle_numeric_character_reference_end_state
#endif
};

void crawler_lexer_init(struct CrawlerInternalLexerContext* lexer) {
    lexer->current_state = CRAWLER_LEXER_STATE_DATA;
    crawler_string_init(&lexer->temporary_buffer);
    lexer->start_tag_emitted = false;
    crawler_string_init(&lexer->last_emitted_start_tag_name);
    lexer->current_attribute_node = NULL;
}

bool crawler_lexer_create(CrawlerLexerContext* lexer) {
    if (!crawler_named_reference_create(lexer->named_ref))
        return false;
    return true;
}

void crawler_lexer_destroy(CrawlerLexerContext* lexer) {
    crawler_named_reference_destroy(lexer->named_ref);
}

CrawlerLexerResult crawler_lexer_gen_token(struct CrawlerInternalParserContext* parser) {
    CrawlerUTF8Stream* is = &parser->is;
    CrawlerLexerResult step_result;
    do {
        if (crawler_stream_get(parser) == CRAWLER_STREAM_MISSING_ELEMENT)
            return CRAWLER_LEXER_MISSING_CP;
        step_result = kCrawlerHandlerDispatchTable[parser->lexer.current_state](parser, is->current_code_point);
    } while (step_result == CRAWLER_LEXER_NEXT_CP);
    return step_result;
}