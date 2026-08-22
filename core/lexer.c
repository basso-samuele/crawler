#include "lexer.h"
#include "parser.h"
#include "stream.h"
#include "attributes.h"
#include "utils.h"
#include "named_ref.h"

#include <stdbool.h>
#include <assert.h>

static bool is_ascii_upper_alpha(int cp) {
    return ((cp >= 0x0041) && (cp <= 0x005A));
}

static bool is_ascii_lower_alpha(int cp) {
    return ((cp >= 0x0061) && (cp <= 0x007A));
}

static bool is_ascii_alpha(int cp) {
    return ((cp >= 0x0041) && (cp <= 0x005A)) ||
           ((cp >= 0x0061) && (cp <= 0x007A));
}

static bool is_ascii_digit(int cp) {
    return (cp >= 0x0030 && cp <= 0x0039);
}

static bool is_ascii_alphanumeric(int cp) {
    return (cp >= 0x0041 && cp <= 0x005A) ||
           (cp >= 0x0061 && cp <= 0x007A) ||
           (cp >= 0x0030 && cp <= 0x0039);
}

static bool is_ascii_lower_hex_digit(int cp) {
    return (cp >= 0x0030 && cp <= 0x0039) ||
           (cp >= 0x0061 && cp <= 0x0066);
}

static bool is_ascii_upper_hex_digit(int cp) {
    return (cp >= 0x0030 && cp <= 0x0039) ||
           (cp >= 0x0041 && cp <= 0x0046);
}

static bool is_ascii_hex_digit(int cp) {
    return (cp >= 0x0030 && cp <= 0x0039) ||
           (cp >= 0x0061 && cp <= 0x0066) ||
           (cp >= 0x0041 && cp <= 0x0046);
}

static bool is_ascii_whitespace(int cp) {
    return (cp == 0x0009) || (cp == 0x000A) ||
           (cp == 0x000C) || (cp == 0x000D) ||
           (cp == 0x0020);
}

static bool is_noncharacter(int cp) {
    return (cp >= 0xFDD0 && cp <= 0xFDEF) ||
           (cp & 0xFFFF) == 0xFFFE ||
           (cp & 0xFFFF) == 0xFFFF;
}

static bool is_surrogate(int cp) {
    return (cp >= 0xD800 && cp <= 0xDBFF) ||
           (cp >= 0xDC00 && cp <= 0xDFFF);
}

static bool is_control(int cp) {
    return (cp >= 0x0000 && cp <= 0x001F) ||
           (cp >= 0x007F && cp <= 0x009F);
}

static void set_return_state(CrawlerParserContext* parser, CrawlerLexerState state) {
    parser->lexer.return_state = state;
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
    crawler_token_destroy(&parser->current_token);
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
    return true;
}

static bool create_start_tag_token(CrawlerParserContext* parser) {
    crawler_token_destroy(&parser->current_token);
    if (!crawler_string_create(&parser->current_token.data.start_tag.name, 8))
        return false;
    parser->current_token.data.start_tag.is_self_closing = false;
    parser->current_token.data.start_tag.attributes = NULL;
    parser->current_token.type = CRAWLER_TOKEN_START_TAG;
    return true;
}

static bool create_end_tag_token(CrawlerParserContext* parser) {
    crawler_token_destroy(&parser->current_token);
    if (!crawler_string_create(&parser->current_token.data.end_tag, 8))
        return false;
    parser->current_token.type = CRAWLER_TOKEN_END_TAG;
    return true;
}

static bool create_comment_token(CrawlerParserContext* parser) {
    crawler_token_destroy(&parser->current_token);
    if (!crawler_string_create(&parser->current_token.data.str, 16))
        return false;
    parser->current_token.type = CRAWLER_TOKEN_COMMENT;
    return true;
}

static bool create_character_token(CrawlerParserContext* parser) {
    crawler_token_destroy(&parser->current_token);
    if (!crawler_string_create(&parser->current_token.data.start_tag.name, 1))
        return false;
    parser->current_token.type = CRAWLER_TOKEN_CHARACTER;
    return true;
}

static void create_eof_token(CrawlerParserContext* parser) {
    crawler_token_destroy(&parser->current_token);
    parser->current_token.type = CRAWLER_TOKEN_EOF;
}

static bool create_processing_instruction_token(CrawlerParserContext* parser) {
    crawler_token_destroy(&parser->current_token);
    if (!crawler_string_create(&parser->current_token.data.proc_in.data, 1))
        return false;
    if (!crawler_string_create(&parser->current_token.data.proc_in.target, 1))
        return false;
    parser->current_token.type = CRAWLER_TOKEN_PROCESSING_INSTRUCTION;
    return true;
}

static bool temporary_to_target(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_PROCESSING_INSTRUCTION);
    crawler_string_destroy(&parser->current_token.data.proc_in.target);
    return crawler_string_clone(&parser->current_token.data.proc_in.target, &parser->lexer.temporary_buffer);
}

static bool manual_emit_character(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_CHARACTER);
    return crawler_string_append(&parser->current_token.data.str, cp);
}

static bool manual_emit_current_character(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_CHARACTER);
    return crawler_string_append(&parser->current_token.data.str, parser->is.current_code_point);
}

static CrawlerLexerResult emit_character(CrawlerParserContext* parser, int cp) {
    if (!create_character_token(parser))
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, cp))
        return CRAWLER_LEXER_FAILURE;
    return CRAWLER_LEXER_SUCCESS;
}

static CrawlerLexerResult emit_current_character(CrawlerParserContext* parser) {
    if (!create_character_token(parser))
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_current_character(parser))
        return CRAWLER_LEXER_FAILURE;
    return CRAWLER_LEXER_SUCCESS;
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
    crawler_debug("Emitted temporary buffer as characters, deleting the buffer.\n");
    temporary_to_empty_string(parser);
    return true;
}

static void set_self_closing(CrawlerParserContext* parser) {
    if (parser->current_token.type == CRAWLER_TOKEN_START_TAG)
        parser->current_token.data.start_tag.is_self_closing = true;
}

static void finalize_current_attribute(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG || parser->current_token.type == CRAWLER_TOKEN_END_TAG);
    if (parser->lexer.current_attribute_node == NULL) {
        crawler_debug("Possibly discarded attribute, not finalizing.\n");
        return;
    }
    if (parser->current_token.type == CRAWLER_TOKEN_START_TAG) {
        crawler_attribute_list_insert(&parser->current_token.data.start_tag.attributes, parser->lexer.current_attribute_node);
        parser->lexer.current_attribute_node = NULL;
    } else {
        crawler_attribute_list_destroy(&parser->lexer.current_attribute_node);
    }
}

static bool start_new_attribute(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG || parser->current_token.type == CRAWLER_TOKEN_END_TAG);
    if (parser->lexer.current_attribute_node != NULL)
        finalize_current_attribute(parser);
    parser->lexer.current_attribute_node = _crawler_alloc(sizeof *parser->lexer.current_attribute_node);
    crawler_attribute_node_init(parser->lexer.current_attribute_node);
    return crawler_attribute_node_create(parser->lexer.current_attribute_node);
}

static bool append_to_current_attribute_name(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG || parser->current_token.type == CRAWLER_TOKEN_END_TAG);
    assert(parser->lexer.current_attribute_node != NULL);
    return crawler_string_append(&parser->lexer.current_attribute_node->attribute.name, cp);
}

static bool append_to_current_attribute_value(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG || parser->current_token.type == CRAWLER_TOKEN_END_TAG);
    if (parser->lexer.current_attribute_node == NULL) {
        crawler_debug("Possibly discarded attribute.\n");
        return true;
    }
    return crawler_string_append(&parser->lexer.current_attribute_node->attribute.value, cp);
}

static bool append_to_comment(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
    return crawler_string_append(&parser->current_token.data.str, cp);
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
    assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG || parser->current_token.type == CRAWLER_TOKEN_END_TAG);
    assert(parser->lexer.current_attribute_node);
    CrawlerAttributeNode* it = parser->current_token.data.start_tag.attributes;
    while (it) {
        bool match =
            crawler_string_compare(&it->attribute.name, &parser->lexer.current_attribute_node->attribute.name);
        if (match) {
            discard_current_attribute(parser);
            return false;
        }
        it = it->next;
    }
    return true;
}

static bool emit_current_tag_token(CrawlerParserContext* parser) {
    switch(parser->current_token.type) {
        case CRAWLER_TOKEN_START_TAG:
            crawler_string_destroy(&parser->lexer.last_emitted_start_tag_name);
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
    crawler_debug("Converted temporary buffer to comment, deleting the buffer.\n");
    temporary_to_empty_string(parser);
    return true;
}

static bool consumed_as_part_of_an_attribute(CrawlerParserContext* parser) {
    return
        (parser->lexer.return_state == CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED) ||
        (parser->lexer.return_state == CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED) ||
        (parser->lexer.return_state == CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_UNQUOTED);
}

static CrawlerLexerResult flush_code_points_consumed_as_a_character_reference(CrawlerParserContext* parser) {
    if (consumed_as_part_of_an_attribute(parser)) {
        if (!crawler_string_append_string_buffer(
            &parser->lexer.current_attribute_node->attribute.value,
            &parser->lexer.temporary_buffer))
            return CRAWLER_LEXER_FAILURE;
        crawler_debug("Appended temporary buffer to attribute value, deleting the buffer.\n");
        temporary_to_empty_string(parser);
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!emit_temporary_buffer_character(parser))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_SUCCESS;
    }
}

static void set_force_quirks(CrawlerParserContext* parser) {
    assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
    parser->current_token.data.doc_type.force_quirks = true;
}

static bool append_doctype_name(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
    return crawler_string_append(&parser->current_token.data.doc_type.name, cp);
}

static bool append_to_public_identifier(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
    return crawler_string_append(&parser->current_token.data.doc_type.public_identifier, cp);
}

static bool append_to_system_identifier(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
    return crawler_string_append(&parser->current_token.data.doc_type.system_identifier, cp);
}

static bool append_to_processing_instr_data(CrawlerParserContext* parser, int cp) {
    assert(parser->current_token.type == CRAWLER_TOKEN_PROCESSING_INSTRUCTION);
    return crawler_string_append(&parser->current_token.data.str, cp);
}

// https://html.spec.whatwg.org/#data-state
static CrawlerLexerResult handle_data_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0026: // AMPERSAND
        set_return_state(parser, CRAWLER_LEXER_STATE_DATA);
        switch_state(parser, CRAWLER_LEXER_STATE_CHARACTER_REFERENCE);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003C: // LESS-THAN-SIGN
        switch_state(parser, CRAWLER_LEXER_STATE_TAG_OPEN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        break;
    case -1: // EOF
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        break;
    }
    return emit_current_character(parser);
}

// https://html.spec.whatwg.org/#rcdata-state
static CrawlerLexerResult handle_rcdata_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x0026: // AMPERSAND
        set_return_state(parser, CRAWLER_LEXER_STATE_RCDATA);
        switch_state(parser, CRAWLER_LEXER_STATE_CHARACTER_REFERENCE);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003C: // LESS-THAN-SIGN
        switch_state(parser, CRAWLER_LEXER_STATE_RCDATA_LESS_THAN_SIGN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        return emit_character(parser, 0xFFFD); // REPLACEMENT CHARACTER
    case -1: // EOF
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#rawtext-state
static CrawlerLexerResult handle_rawtext_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x003C: // LESS-THAN-SIGN
        switch_state(parser, CRAWLER_LEXER_STATE_RAWTEXT_LESS_THAN_SIGN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        return emit_character(parser, 0xFFFD); // REPLACEMENT CHARACTER
    case -1: // EOF
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#script-data-state
static CrawlerLexerResult handle_script_data_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003C: // LESS-THAN-SIGN
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_LESS_THAN_SIGN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        return emit_character(parser, 0xFFFD); // REPLACEMENT CHARACTER
    case -1: // EOF
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#plaintext-state
static CrawlerLexerResult handle_plaintext_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        return emit_character(parser, 0xFFFD); // REPLACEMENT CHARACTER
    case -1: // EOF
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#tag-open-state
static CrawlerLexerResult handle_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0021: // EXCLAMATION MARK (!)
        switch_state(parser, CRAWLER_LEXER_STATE_MARKUP_DECLARATION_OPEN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
        switch_state(parser, CRAWLER_LEXER_STATE_END_TAG_OPEN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003F: // QUESTION MARK (?)
        temporary_to_empty_string(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_PROCESSING_INSTRUCTION_OPEN);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_BEFORE_TAG_NAME);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        return emit_character(parser, 0x003C); // LESS-THAN SIGN
    default:
        if (is_ascii_alpha(cp)) { // ASCII alpha
            if (!create_start_tag_token(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_state(parser, CRAWLER_LEXER_STATE_TAG_NAME);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else { // Anything else
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
            switch_state(parser, CRAWLER_LEXER_STATE_DATA);
            stream_reconsume(parser);
            return emit_character(parser, 0x003C); // LESS-THAN SIGN
        }
    }
}

// https://html.spec.whatwg.org/#end-tag-open-state
static CrawlerLexerResult handle_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_END_TAG_NAME);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_BEFORE_TAG_NAME);
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN 
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_alpha(cp)) { // ASCII alpha
            if (!create_end_tag_token(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_state(parser, CRAWLER_LEXER_STATE_TAG_NAME);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else { // Anything else
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME);
            if (!create_comment_token(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_COMMENT);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    }
}

// https://html.spec.whatwg.org/#tag-name-state
static CrawlerLexerResult handle_tag_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
        switch_state(parser, CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        return emit_current_tag_token(parser);
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_tag_name(parser, 0xFFFD)) // REPLACEMENT CHARACTER
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_upper_alpha(cp)) // ASCII upper alpha
            cp += 0x0020;
        if (!append_tag_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#rcdata-less-than-sign-state
static CrawlerLexerResult handle_rcdata_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        temporary_to_empty_string(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_RCDATA_END_TAG_OPEN);
        
        return CRAWLER_LEXER_NEXT_CP;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_RCDATA);
        stream_reconsume(parser);
        return emit_character(parser, 0x003C); // LESS-THAN SIGN
    }
}

// https://html.spec.whatwg.org/#rcdata-end-tag-open-state
static CrawlerLexerResult handle_rcdata_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    stream_reconsume(parser);
    if (is_ascii_alpha(cp)) { // ASCII alpha
        if (!create_end_tag_token(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_RCDATA_END_TAG_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    } else { // Anything else
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN 
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_RCDATA);
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
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
            
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG);
            
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_DATA);
            
            return emit_current_tag_token(parser);
        }
        break;
    default:
        if (!is_ascii_alpha(cp))
            break;
        if (is_ascii_upper_alpha(cp))
            cp += 0x0020;
        if (!append_tag_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        if (!temporary_append(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        
        return CRAWLER_LEXER_NEXT_CP;
    }

    if (!create_character_token(parser))
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
        return CRAWLER_LEXER_FAILURE;
    if (!emit_temporary_buffer_character(parser))
        return CRAWLER_LEXER_FAILURE;

    switch_state(parser, CRAWLER_LEXER_STATE_RCDATA);
    stream_reconsume(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#rawtext-less-than-sign-state
static CrawlerLexerResult handle_rawtext_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        temporary_to_empty_string(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_RAWTEXT_END_TAG_OPEN);
        
        return CRAWLER_LEXER_NEXT_CP;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_RAWTEXT);
        stream_reconsume(parser);
        return emit_character(parser, 0x003C); // LESS-THAN SIGN
    }
}

// https://html.spec.whatwg.org/#rawtext-end-tag-open-state
static CrawlerLexerResult handle_rawtext_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    stream_reconsume(parser);
    if (is_ascii_alpha(cp)) {
        if (!create_end_tag_token(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_RAWTEXT_END_TAG_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
            return CRAWLER_LEXER_FAILURE;

        switch_state(parser, CRAWLER_LEXER_STATE_RAWTEXT);
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
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
            
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG);
            
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_DATA);
            
            return emit_current_tag_token(parser);
        }
        break;
    default:
        if (!is_ascii_alpha(cp))
            break;
        if (is_ascii_upper_alpha(cp))
            cp += 0x0020;
        if (!temporary_append(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        
        return CRAWLER_LEXER_NEXT_CP;
    }
    // Anything else
    if (!create_character_token(parser))
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
        return CRAWLER_LEXER_FAILURE;

    if (!emit_temporary_buffer_character(parser))
        return CRAWLER_LEXER_FAILURE;

    switch_state(parser, CRAWLER_LEXER_STATE_RAWTEXT);
    stream_reconsume(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#script-data-less-than-sign-state
static CrawlerLexerResult handle_script_data_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        temporary_to_empty_string(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_END_TAG_OPEN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0021: // EXCLAMATION MARK (!)
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x0021)) // EXCLAMATION MARK
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPE_START);
        return CRAWLER_LEXER_SUCCESS;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA);
        stream_reconsume(parser);
        return emit_character(parser, 0x003C); // LESS-THAN SIGN
    }
}

// https://html.spec.whatwg.org/#script-data-end-tag-open-state
static CrawlerLexerResult handle_script_data_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    stream_reconsume(parser);
    if (is_ascii_alpha(cp)) {
        if (!create_end_tag_token(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_END_TAG_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
            return CRAWLER_LEXER_FAILURE;

        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA);
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
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
            
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG);
            
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_DATA);
            
            return emit_current_tag_token(parser);
        }
        break;
    default:
        if (!is_ascii_alpha(cp))
            break;
        if (is_ascii_upper_alpha(cp))
            cp += 0x0020;
        if (!append_tag_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        if (!temporary_append(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        
        return CRAWLER_LEXER_NEXT_CP;
    }

    if (!create_character_token(parser))
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
        return CRAWLER_LEXER_FAILURE;

    if (!emit_temporary_buffer_character(parser))
        return CRAWLER_LEXER_FAILURE;
    switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA);
    stream_reconsume(parser);
    return CRAWLER_LEXER_SUCCESS;
}

// https://html.spec.whatwg.org/#script-data-escape-start-state
static CrawlerLexerResult handle_script_data_escape_start_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPE_START_DASH);
        
        return emit_character(parser, 0x002D); // HYPHEN-MINUS (-)
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#script-data-escape-start-dash-state
static CrawlerLexerResult handle_script_data_escape_start_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH);
        return emit_character(parser, 0x002D); // HYPHEN-MINUS (-)
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-state
static CrawlerLexerResult handle_script_data_escaped_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_DASH);
        return emit_character(parser, 0x002D); // HYPHEN-MINUS (-)
    case 0x003C: // LESS-THAN SIGN (<)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        return emit_character(parser, 0xFFFD); // REPLACEMENT CHARACTER
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT);
        return emit_current_character(parser);
    default:
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-dash-state
static CrawlerLexerResult handle_script_data_escaped_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_DASH_DASH);
        return emit_character(parser, 0x002D); // HYPHEN-MINUS (-)
    case 0x003C: // LESS-THAN SIGN (<)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        return emit_character(parser, 0xFFFD); // REPLACEMENT CHARACTER
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-dash-dash-state
static CrawlerLexerResult handle_script_data_escaped_dash_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        return emit_character(parser, 0x002D);
    case 0x003C: // LESS-THAN SIGN (<)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_LESS_THAN_SIGN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA);
        return emit_character(parser, 0x003E);
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        return emit_character(parser, 0xFFFD); // REPLACEMENT CHARACTER
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-less-than-sign-state
static CrawlerLexerResult handle_script_data_escaped_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        temporary_to_empty_string(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_OPEN);
        
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (is_ascii_alpha(cp)) {
            temporary_to_empty_string(parser);
            switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_START);
        } else {
            switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        }
        stream_reconsume(parser);
        return emit_character(parser, 0x003C);
    }
}

// https://html.spec.whatwg.org/#script-data-escaped-end-tag-open-state
static CrawlerLexerResult handle_script_data_escaped_end_tag_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    stream_reconsume(parser);
    if (is_ascii_alpha(cp)) {
        create_end_tag_token(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED_END_TAG_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
            return CRAWLER_LEXER_FAILURE;

        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
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
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x002F: // SOLIDUS (/)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG);
            return CRAWLER_LEXER_NEXT_CP;
        }
        break;
    case 0x003E: // GREATER-THAN SIGN (>)
        if (is_appropriate_end_tag_token(parser)) {
            switch_state(parser, CRAWLER_LEXER_STATE_DATA);
            return emit_current_tag_token(parser);
        }
        break;
    default:
        // Possible future improvement: the append method may lowercase each code point.
        if (!is_ascii_alpha(cp))
            break;
        if (is_ascii_upper_alpha(cp))
            cp += 0x0020;
        if (!append_tag_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        if (!temporary_append(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        
        return CRAWLER_LEXER_NEXT_CP;
    }
    // Anything else
    if (!create_character_token(parser))
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x003C)) // LESS-THAN SIGN
        return CRAWLER_LEXER_FAILURE;
    if (!manual_emit_character(parser, 0x002F)) // SOLIDUS
        return CRAWLER_LEXER_FAILURE;

    if (!emit_temporary_buffer_character(parser))
        return CRAWLER_LEXER_FAILURE;

    switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
    stream_reconsume(parser);
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
        if (crawler_string_compare_with_literal(&parser->lexer.temporary_buffer, "script", 6)) {
            switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
        } else {
            switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        }
        
        return emit_current_character(parser);
    default:
        if (is_ascii_alpha(cp)) {
            if (is_ascii_upper_alpha(cp))
                cp += 0x0020;
            temporary_append(parser, cp);
            
            return emit_current_character(parser);
        }

        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-state
static CrawlerLexerResult handle_script_data_double_escaped_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH);
        return emit_character(parser, 0x002D);
    case 0x003C: // LESS-THAN SIGN (<)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN);
        return emit_character(parser, 0x003C);
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        return emit_character(parser, 0xFFFD);
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-dash-state
static CrawlerLexerResult handle_script_data_double_escaped_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_DASH_DASH);
        return emit_character(parser, 0x002D);
    case 0x003C: // LESS-THAN SIGN (<)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN);
        return emit_character(parser, 0x003C);
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
        return emit_character(parser, 0xFFFD);
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-dash-dash-state
static CrawlerLexerResult handle_script_data_double_escaped_dash_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        return emit_character(parser, 0x002D);
    case 0x003C: // LESS-THAN SIGN (<)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED_LESS_THAN_SIGN);
        return emit_character(parser, 0x003C);
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA);
        return emit_character(parser, 0x003E);
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
        return emit_character(parser, 0xFFFD);
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_SCRIPT_HTML_COMMENT_LIKE_TEXT);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#script-data-double-escaped-less-than-sign-state
static CrawlerLexerResult handle_script_data_double_escaped_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002F: // SOLIDUS (/)
        temporary_to_empty_string(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPE_END);
        
        return emit_character(parser, 0x002F);
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
        stream_reconsume(parser);
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
        if (crawler_string_compare_with_literal(&parser->lexer.temporary_buffer, "script", 6)) {
            switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_ESCAPED);
        } else {
            switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
        }
        
        return emit_current_character(parser);
    default:
        if (is_ascii_alpha(cp)) {
            if (is_ascii_upper_alpha(cp))
                cp += 0x0020;
            if (!temporary_append(parser, cp))
                return CRAWLER_LEXER_FAILURE;
            
            return emit_current_character(parser);
        } else {
            switch_state(parser, CRAWLER_LEXER_STATE_SCRIPT_DATA_DOUBLE_ESCAPED);
            stream_reconsume(parser);
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
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
    case 0x003E: // GREATER-THAN SIGN (>)
    case -1: // EOF
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_NAME);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003D: // EQUALS SIGN (=)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_EQUALS_SIGN_BEFORE_ATTRIBUTE_NAME);
        if (!start_new_attribute(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!append_to_current_attribute_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_NAME);
        
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (!start_new_attribute(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_NAME);
        stream_reconsume(parser);
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
        if (!check_current_attribute_unique(parser))
            crawler_debug("Discarded duplicated attribute.\n");
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_NAME);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003D: // EQUALS SIGN (=)
        if (!check_current_attribute_unique(parser))
            crawler_debug("Discarded duplicated attribute.\n");
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_VALUE);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_current_attribute_name(parser, 0xFFFD))
            return CRAWLER_LEXER_FAILURE;
        
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
    case 0x0027: // APOSTROPHE (')
    case 0x003C: // LESS-THAN SIGN (<)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_CHARACTER_IN_ATTRIBUTE_NAME);
        if (!append_to_current_attribute_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (is_ascii_upper_alpha(cp))
            cp += 0x0020;
        if (!append_to_current_attribute_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-attribute-name-state
static CrawlerLexerResult handle_after_attribute_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009:
    case 0x000A:
    case 0x000C:
    case 0x0020:
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F:
        switch_state(parser, CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG);
        finalize_current_attribute(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003D:
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_VALUE);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E:
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        finalize_current_attribute(parser);
        return emit_current_tag_token(parser);
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!start_new_attribute(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_NAME);
        stream_reconsume(parser);
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
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
        switch_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        switch_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_ATTRIBUTE_VALUE);
        finalize_current_attribute(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        return emit_current_tag_token(parser);
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_UNQUOTED);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#attribute-value-(double-quoted)-state
static CrawlerLexerResult handle_attribute_value_double_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x0022: // QUOTATION MARK (")
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0026: // AMPERSAND (&)
        set_return_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_DOUBLE_QUOTED);
        switch_state(parser, CRAWLER_LEXER_STATE_CHARACTER_REFERENCE);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_current_attribute_value(parser, 0xFFFD))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_current_attribute_value(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#attribute-value-(single-quoted)-state
static CrawlerLexerResult handle_attribute_value_single_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0027: // APOSTROPHE (')
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_ATTRIBUTE_VALUE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0026: // AMPERSAND (&)
        set_return_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_SINGLE_QUOTED);
        switch_state(parser, CRAWLER_LEXER_STATE_CHARACTER_REFERENCE);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_current_attribute_value(parser, 0xFFFD))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_current_attribute_value(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#attribute-value-(unquoted)-state
static CrawlerLexerResult handle_attribute_value_unquoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        finalize_current_attribute(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0026: // AMPERSAND (&)
        set_return_state(parser, CRAWLER_LEXER_STATE_ATTRIBUTE_VALUE_UNQUOTED);
        switch_state(parser, CRAWLER_LEXER_STATE_CHARACTER_REFERENCE);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        finalize_current_attribute(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        return emit_current_tag_token(parser);
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_current_attribute_value(parser, 0xFFFD))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
    case 0x0027: // APOSTROPHE (')
    case 0x003C: // LESS-THAN SIGN (<)
    case 0x003D: // EQUALS SIGN (=)
    case 0x0060: // GRAVE ACCENT (`)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_CHARACTER_IN_UNQUOTED_ATTRIBUTE_VALUE);
        if (!append_to_current_attribute_value(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG);
        // I expect to have a tag token here and possibly a current_attribute_node.
        finalize_current_attribute(parser);
        crawler_token_destroy(&parser->current_token);

        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_current_attribute_value(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-attribute-value-(quoted)-state
static CrawlerLexerResult handle_after_attribute_value_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    finalize_current_attribute(parser);
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002F: // SOLIDUS (/)
        switch_state(parser, CRAWLER_LEXER_STATE_SELF_CLOSING_START_TAG);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        return emit_current_tag_token(parser);
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG);
        // If this line is reached then a start_tag token should exist.
        assert(parser->current_token.type == CRAWLER_TOKEN_START_TAG);
        // Destroy it before creating an eof token.
        crawler_token_destroy(&parser->current_token);

        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_BETWEEN_ATTRIBUTES);
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#self-closing-start-tag-state
static CrawlerLexerResult handle_self_closing_start_tag_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        set_self_closing(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        
        return emit_current_tag_token(parser);
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG);
        create_eof_token(parser);
        
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_SOLIDUS_IN_TAG);
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_ATTRIBUTE_NAME);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#bogus-comment-state
static CrawlerLexerResult handle_bogus_comment_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        // If current token is comment returning CRAWLER_LEXER_SUCCESS will emit.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        // If current token is comment returning CRAWLER_LEXER_SUCCESS will emit.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_comment(parser, 0xFFFD))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (!append_to_comment(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#markup-declaration-open-state
static CrawlerLexerResult handle_markup_declaration_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    CrawlerUTF8Stream* stream = &parser->is;

    if (crawler_stream_consume_match(stream, "--", 2, false)) {
        if (!create_comment_token(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_START);
        return CRAWLER_LEXER_NEXT_CP;
    } else if (crawler_stream_consume_match(stream, "DOCTYPE", 7, false)) {
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE);
        return CRAWLER_LEXER_NEXT_CP;
    } else if (crawler_stream_consume_match(stream, "[CDATA[", 7, false)) {
        // Missing stack of open elements implementation.
        assert(false);
    } else {
        crawler_parser_register_error(parser, CRAWLER_ERROR_INCORRECTLY_OPENED_COMMENT);
        if (!create_comment_token(parser))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-start-state
static CrawlerLexerResult handle_comment_start_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_START_DASH);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-start-dash-state
static CrawlerLexerResult handle_comment_start_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_END);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABRUPT_CLOSING_OF_EMPTY_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_COMMENT);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-state
static CrawlerLexerResult handle_comment_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003C: // LESS-THAN SIGN (<)
        if (!append_to_comment(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_LESS_THAN_SIGN);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_END_DASH);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_comment(parser, 0xFFFD))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_comment(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-less-than-sign-state
static CrawlerLexerResult handle_comment_less_than_sign_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0021: // EXCLAMATION MARK (!)
        if (!append_to_comment(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_LESS_THAN_SIGN_BANG);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003C: // LESS-THAN SIGN (<)
        if (!append_to_comment(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-less-than-sign-bang-state
static CrawlerLexerResult handle_comment_less_than_sign_bang_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_LESS_THAN_SIGN_BANG_DASH);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-less-than-sign-bang-dash-state
static CrawlerLexerResult handle_comment_less_than_sign_bang_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_LESS_THAN_SIGN_BANG_DASH_DASH);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_END_DASH);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-less-than-sign-bang-dash-dash-state
static CrawlerLexerResult handle_comment_less_than_sign_bang_dash_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
    case -1: // EOF
        break;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_NESTED_COMMENT);
        break;
    }

    switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_END);
    stream_reconsume(parser);
    return CRAWLER_LEXER_NEXT_CP;
}

// https://html.spec.whatwg.org/#comment-end-dash-state
static CrawlerLexerResult handle_comment_end_dash_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_END);
        return CRAWLER_LEXER_NEXT_CP;
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-end-state
static CrawlerLexerResult handle_comment_end_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    case 0x0021: // EXCLAMATION MARK (!)
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_END_BANG);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x002D: // HYPHEN-MINUS (-)
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#comment-end-bang-state
static CrawlerLexerResult handle_comment_end_bang_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x002D: // HYPHEN-MINUS (-)
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        if (!append_to_comment(parser, 0x0021)) // EXCLAMATION MARK (!)
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT_END_DASH);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_INCORRECTLY_CLOSED_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_COMMENT);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        // Emit current comment token.
        assert(parser->current_token.type == CRAWLER_TOKEN_COMMENT);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        if (!append_to_comment(parser, 0x002D)) // HYPHEN-MINUS (-)
            return CRAWLER_LEXER_FAILURE;
        if (!append_to_comment(parser, 0x0021)) // EXCLAMATION MARK (!)
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_COMMENT);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#doctype-state
static CrawlerLexerResult handle_doctype_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_DOCTYPE_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_DOCTYPE_NAME);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        if (!create_doctype_token(parser))
            return CRAWLER_LEXER_FAILURE;
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        // Emit current token (DOCTYPE).
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_BEFORE_DOCTYPE_NAME);
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_DOCTYPE_NAME);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#before-doctype-name-state
static CrawlerLexerResult handle_before_doctype_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!create_doctype_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!append_doctype_name(parser, 0xFFFD)) // REPLACEMENT CHARACTER
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_DOCTYPE_NAME);
        if (!create_doctype_token(parser))
            return CRAWLER_LEXER_FAILURE;
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        // Emit current token (DOCTYPE).
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        if (!create_doctype_token(parser))
            return CRAWLER_LEXER_FAILURE;
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        // Emit current token (DOCTYPE).
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!create_doctype_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (is_ascii_upper_alpha(cp))
            cp += 0x0020;
        if (!append_doctype_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#doctype-name-state
static CrawlerLexerResult handle_doctype_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_DOCTYPE_NAME);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_doctype_name(parser, 0xFFFD))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_upper_alpha(cp))
            cp += 0x0020;
        if (!append_doctype_name(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-doctype-name-state
static CrawlerLexerResult handle_after_doctype_name_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (crawler_stream_consume_match(&parser->is, "PUBLIC", 6, false)) {
            switch_state(parser, CRAWLER_LEXER_STATE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
        } else if (crawler_stream_consume_match(&parser->is, "SYSTEM", 6, false)) {
            switch_state(parser, CRAWLER_LEXER_STATE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_CHARACTER_SEQUENCE_AFTER_DOCTYPE_NAME);
            set_force_quirks(parser);
            switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
            stream_reconsume(parser);
        }
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-doctype-public-keyword-state
static CrawlerLexerResult handle_after_doctype_public_keyword_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
        parser->current_token.data.doc_type.has_public_identifier = true;
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        parser->current_token.data.doc_type.has_public_identifier = true;
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_PUBLIC_KEYWORD);
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#before-doctype-public-identifier-state
static CrawlerLexerResult handle_before_doctype_public_identifier_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
        parser->current_token.data.doc_type.has_public_identifier = true;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_PUBLIC_IDENTIFIER_DOUBLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        parser->current_token.data.doc_type.has_public_identifier = true;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_PUBLIC_IDENTIFIER_SINGLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_DOCTYPE_PUBLIC_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_PUBLIC_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#doctype-public-identifier-(double-quoted)-state
static CrawlerLexerResult handle_doctype_public_identifier_double_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0022: // QUOTATION MARK (")
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_DOCTYPE_PUBLIC_IDENTIFIER);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_public_identifier(parser, 0xFFFD)) // REPLACEMENT CHARACTER
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_public_identifier(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#doctype-public-identifier-(single-quoted)-state
static CrawlerLexerResult handle_doctype_public_identifier_single_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0027: // APOSTROPHE (')
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_DOCTYPE_PUBLIC_IDENTIFIER);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_public_identifier(parser, 0xFFFD)) // REPLACEMENT CHARACTER
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABRUPT_DOCTYPE_PUBLIC_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_public_identifier(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-doctype-public-identifier-state
static CrawlerLexerResult handle_after_doctype_public_identifier_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        switch_state(parser, CRAWLER_LEXER_STATE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case 0x0022: // QUOTATION MARK (")
        parser->current_token.data.doc_type.has_system_identifier = true;
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS);
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        parser->current_token.data.doc_type.has_system_identifier = true;
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_BETWEEN_DOCTYPE_PUBLIC_AND_SYSTEM_IDENTIFIERS);
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#between-doctype-public-and-system-identifiers-state
static CrawlerLexerResult handle_between_doctype_public_and_system_identifiers_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case 0x0022: // QUOTATION MARK (")
        parser->current_token.data.doc_type.has_system_identifier = true;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        parser->current_token.data.doc_type.has_system_identifier = true;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-doctype-system-keyword-state
static CrawlerLexerResult handle_after_doctype_system_keyword_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        switch_state(parser, CRAWLER_LEXER_STATE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
        parser->current_token.data.doc_type.has_system_identifier = true;
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        parser->current_token.data.doc_type.has_system_identifier = true;
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_WHITESPACE_AFTER_DOCTYPE_SYSTEM_KEYWORD);
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#before-doctype-system-identifier-state
static CrawlerLexerResult handle_before_doctype_system_identifier_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0022: // QUOTATION MARK (")
        parser->current_token.data.doc_type.has_system_identifier = true;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_DOUBLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0027: // APOSTROPHE (')
        parser->current_token.data.doc_type.has_system_identifier = true;
        switch_state(parser, CRAWLER_LEXER_STATE_DOCTYPE_SYSTEM_IDENTIFIER_SINGLE_QUOTED);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_QUOTE_BEFORE_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#doctype-system-identifier-(double-quoted)-state
static CrawlerLexerResult handle_doctype_system_identifier_double_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0022: // QUOTATION MARK (")
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_DOCTYPE_SYSTEM_IDENTIFIER);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_system_identifier(parser, 0xFFFD)) // REPLACEMENT CHARACTER
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABRUPT_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_system_identifier(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#doctype-system-identifier-(single-quoted)-state
static CrawlerLexerResult handle_doctype_system_identifier_single_quoted_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0027: // APOSTROPHE (')
        switch_state(parser, CRAWLER_LEXER_STATE_AFTER_DOCTYPE_SYSTEM_IDENTIFIER);
        return CRAWLER_LEXER_NEXT_CP;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        if (!append_to_system_identifier(parser, 0xFFFD)) // REPLACEMENT CHARACTER
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABRUPT_DOCTYPE_SYSTEM_IDENTIFIER);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_system_identifier(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-doctype-system-identifier-state
static CrawlerLexerResult handle_after_doctype_system_identifier_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        return CRAWLER_LEXER_NEXT_CP;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_DOCTYPE);
        set_force_quirks(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_CHARACTER_AFTER_DOCTYPE_SYSTEM_IDENTIFIER);
        switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_DOCTYPE);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#bogus-doctype-state
static CrawlerLexerResult handle_bogus_doctype_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        stream_reconsume(parser);
        assert(parser->current_token.type == CRAWLER_TOKEN_DOCTYPE);
        return CRAWLER_LEXER_SUCCESS;
    default:
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#cdata-section-state
static CrawlerLexerResult handle_cdata_section_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x005D: // RIGHT SQUARE BRACKET (])
        switch_state(parser, CRAWLER_LEXER_STATE_CDATA_SECTION_BRACKET);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_CDATA);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        return emit_current_character(parser);
    }
}

// https://html.spec.whatwg.org/#cdata-section-bracket-state
static CrawlerLexerResult handle_cdata_section_bracket_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x005D: // RIGHT SQUARE BRACKET (])
        switch_state(parser, CRAWLER_LEXER_STATE_CDATA_SECTION_END);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_CDATA_SECTION);
        stream_reconsume(parser);
        return emit_character(parser, 0x005D); // RIGHT SQUARE BRACKET (])
    }
}

// https://html.spec.whatwg.org/#cdata-section-end-state
static CrawlerLexerResult handle_cdata_section_end_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x005D: // RIGHT SQUARE BRACKET (])
        return emit_character(parser, 0x005D);
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (!create_character_token(parser))
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x005D)) // RIGHT SQUARE BRACKET (])
            return CRAWLER_LEXER_FAILURE;
        if (!manual_emit_character(parser, 0x005D)) // RIGHT SQUARE BRACKET (])
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_CDATA_SECTION);
        stream_reconsume(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

// https://html.spec.whatwg.org/#processing-instruction-open-state
static CrawlerLexerResult handle_processing_instruction_open_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case -1:
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_PROCESSING_INSTRUCTION);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_alpha(cp) || cp == 0x005F) { // LOW LINE (_)
            switch_state(parser, CRAWLER_LEXER_STATE_PROCESSING_INSTRUCTION_TARGET);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_PROCESSING_INSTRUCTION_TARGET);
            if (!create_comment_token(parser))
                return CRAWLER_LEXER_FAILURE;
            if (!convert_temporary_to_comment(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_COMMENT);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    }
}

// https://html.spec.whatwg.org/#processing-instruction-target-state
static CrawlerLexerResult handle_processing_instruction_target_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
    case 0x003F: // QUESTION MARK (?)
    case 0x003E: // GREATER-THAN SIGN (>)
        CrawlerString* target =
            &parser->lexer.temporary_buffer;

        if (crawler_string_compare_with_literal_ins(target, "xml", 3) ||
            crawler_string_compare_with_literal_ins(target, "xml-stylesheet", 14)) {
            crawler_parser_register_error(parser, CRAWLER_ERROR_DISALLOWED_PROCESSING_INSTRUCTION_TARGET);
            if (!convert_temporary_to_comment(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_COMMENT);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else {
            if (!create_processing_instruction_token(parser))
                return CRAWLER_LEXER_FAILURE;
            if (!temporary_to_target(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_state(parser, CRAWLER_LEXER_STATE_AFTER_PROCESSING_INSTRUCTION_TARGET);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_PROCESSING_INSTRUCTION);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (is_ascii_alphanumeric(cp) || cp == 0x002D || cp == 0x005F) { // HYPHEN-MINUS (-) LOW LINE (_)
            if (!temporary_append(parser, cp))
                return CRAWLER_LEXER_FAILURE;
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_PROCESSING_INSTRUCTION_TARGET);
            if (!convert_temporary_to_comment(parser))
                return CRAWLER_LEXER_FAILURE;
            switch_state(parser, CRAWLER_LEXER_STATE_BOGUS_COMMENT);
            stream_reconsume(parser);
        }
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#after-processing-instruction-target-state
static CrawlerLexerResult handle_after_processing_instruction_target_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x0009: // CHARACTER TABULATION (tab)
    case 0x000A: // LINE FEED (LF)
    case 0x000C: // FORM FEED (FF)
    case 0x0020: // SPACE
        return CRAWLER_LEXER_NEXT_CP;
    default:
        switch_state(parser, CRAWLER_LEXER_STATE_PROCESSING_INSTRUCTION_DATA);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#processing-instruction-data-state
static CrawlerLexerResult handle_processing_instruction_data_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003F: // QUESTION MARK (?)
        switch_state(parser, CRAWLER_LEXER_STATE_PROCESSING_INSTRUCTION_QUESTIONABLE);
        return CRAWLER_LEXER_SUCCESS;
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_PROCESSING_INSTRUCTION);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_PROCESSING_INSTRUCTION);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_processing_instr_data(parser, cp))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#processing-instruction-questionable-state
static CrawlerLexerResult handle_processing_instruction_questionable_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003E: // GREATER-THAN SIGN (>)
        switch_state(parser, CRAWLER_LEXER_STATE_DATA);
        assert(parser->current_token.type == CRAWLER_TOKEN_PROCESSING_INSTRUCTION);
        return CRAWLER_LEXER_SUCCESS;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_PROCESSING_INSTRUCTION);
        create_eof_token(parser);
        return CRAWLER_LEXER_SUCCESS;
    default:
        if (!append_to_processing_instr_data(parser, 0x003F)) // (?)
            return CRAWLER_LEXER_FAILURE;
        switch_state(parser, CRAWLER_LEXER_STATE_PROCESSING_INSTRUCTION_DATA);
        stream_reconsume(parser);
        return CRAWLER_LEXER_NEXT_CP;
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
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (is_ascii_alphanumeric(cp)) {
            switch_state(parser, CRAWLER_LEXER_STATE_NAMED_CHARACTER_REFERENCE);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else {
            switch_to_return_state(parser);
            stream_reconsume(parser);
            return flush_code_points_consumed_as_a_character_reference(parser);
        }
    }
}

// https://html.spec.whatwg.org/#named-character-reference-state
static CrawlerLexerResult handle_named_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    int next_input_character;
    CrawlerStreamResult next_input_character_result =
        crawler_stream_peek(parser, &next_input_character);
    if (next_input_character_result == CRAWLER_STREAM_ERROR)
        return CRAWLER_LEXER_FAILURE;
    if (next_input_character_result == CRAWLER_STREAM_MISSING_ELEMENT) {
        stream_reconsume(parser);
        return CRAWLER_LEXER_MISSING_CP;
    }

    CrawlerCharacterReference cr;
    CrawlerNamedReferenceResult cr_result = crawler_named_reference_step(parser, cp, &cr);

    switch(cr_result) {
    case CRAWLER_CR_SUCCESS:
        stream_reset(parser);
        // Recalculating next_input_character
        CrawlerStreamResult next_input_character_result =
            crawler_stream_peek(parser, &next_input_character);
        if (next_input_character_result == CRAWLER_STREAM_ERROR)
            return CRAWLER_LEXER_FAILURE;
        if (next_input_character_result == CRAWLER_STREAM_MISSING_ELEMENT) {
            stream_reconsume(parser);
            return CRAWLER_LEXER_MISSING_CP;
        }

        int last_character_matched = parser->lexer.temporary_buffer.data[parser->lexer.temporary_buffer.length-1];
        bool historical =
            consumed_as_part_of_an_attribute(parser) &&
            last_character_matched != 0x003B && // SEMICOLON (;)
            (next_input_character == 0x003D || is_ascii_alphanumeric(next_input_character)); // EQUALS SIGN (=)
        switch_to_return_state(parser);
        if (historical) {
            return flush_code_points_consumed_as_a_character_reference(parser);
        } else {
            if (last_character_matched != 0x003B) // SEMICOLON (;)
                crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            temporary_to_empty_string(parser);
            assert(cr.first != 0 || cr.second != 0);
            if (cr.first != 0)
                if (!crawler_string_append(&parser->lexer.temporary_buffer, cr.first))
                    return CRAWLER_LEXER_FAILURE;
            if (cr.second != 0)
                if (!crawler_string_append(&parser->lexer.temporary_buffer, cr.second))
                    return CRAWLER_LEXER_FAILURE;
            return flush_code_points_consumed_as_a_character_reference(parser);
        }
    case CRAWLER_CR_FAILURE:
        stream_reconsume(parser);
        switch_state(parser, CRAWLER_LEXER_STATE_AMBIGUOUS_AMPERSAND);
        return flush_code_points_consumed_as_a_character_reference(parser);
    case CRAWLER_CR_NEXT_CP:
        if (!temporary_append(parser, crawler_named_reference_get_last_matched_char(parser)))
            return CRAWLER_LEXER_FAILURE;
        return CRAWLER_LEXER_NEXT_CP;
    }
}

// https://html.spec.whatwg.org/#ambiguous-ampersand-state
static CrawlerLexerResult handle_ambiguous_ampersand_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003B: // SEMICOLON (;)
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNKNOWN_NAMED_CHARACTER_REFERENCE);
        stream_reconsume(parser);
        switch_to_return_state(parser);
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (is_ascii_alphanumeric(cp)) {
            if (consumed_as_part_of_an_attribute(parser)) {
                if (!append_to_current_attribute_value(parser, cp))
                    return CRAWLER_LEXER_FAILURE;
                return CRAWLER_LEXER_NEXT_CP;
            } else {
                return emit_current_character(parser);
            }
        } else {
            stream_reconsume(parser);
            switch_to_return_state(parser);
            return CRAWLER_LEXER_NEXT_CP;
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
        return CRAWLER_LEXER_NEXT_CP;
    default:
        stream_reconsume(parser);
        if (is_ascii_digit(cp)) {
            switch_state(parser, CRAWLER_LEXER_STATE_DECIMAL_CHARACTER_REFERENCE);
            return CRAWLER_LEXER_NEXT_CP;
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
            switch_to_return_state(parser);
            return flush_code_points_consumed_as_a_character_reference(parser);
        }
    }
}

// https://html.spec.whatwg.org/#hexadecimal-character-reference-start-state
static CrawlerLexerResult handle_hexadecimal_character_reference_start_state(struct CrawlerInternalParserContext* parser, int cp) {
    stream_reconsume(parser);
    if (is_ascii_hex_digit(cp)) {
        switch_state(parser, CRAWLER_LEXER_STATE_HEXADECIMAL_CHARACTER_REFERENCE);
        return CRAWLER_LEXER_NEXT_CP;
    } else {
        crawler_parser_register_error(parser, CRAWLER_ERROR_ABSENCE_OF_DIGITS_IN_NUMERIC_CHARACTER_REFERENCE);
        switch_to_return_state(parser);
        return flush_code_points_consumed_as_a_character_reference(parser);
    }
}

static void multiply_hex_by_16(int* value) {
    if (*value > UINT32_MAX / 16)
        *value = UINT32_MAX;
    else
        *value *= 16;
}

static void add_to_value_safe(int* value, int add) {
    if (*value > UINT32_MAX - add)
        *value = UINT32_MAX;
    else
        *value += add;
}

// https://html.spec.whatwg.org/#hexadecimal-character-reference-state
static CrawlerLexerResult handle_hexadecimal_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003B: // SEMICOLON (;)
        switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
        
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (is_ascii_digit(cp)) {
            multiply_hex_by_16(&parser->lexer.character_reference_code);
            add_to_value_safe(&parser->lexer.character_reference_code, cp-0x0030);
            return CRAWLER_LEXER_NEXT_CP;
        } else if (is_ascii_upper_hex_digit(cp)) {
            multiply_hex_by_16(&parser->lexer.character_reference_code);
            add_to_value_safe(&parser->lexer.character_reference_code, cp-0x0037);
            return CRAWLER_LEXER_NEXT_CP;
        } else if (is_ascii_lower_hex_digit(cp)) {
            multiply_hex_by_16(&parser->lexer.character_reference_code);
            add_to_value_safe(&parser->lexer.character_reference_code, cp-0x0057);
            return CRAWLER_LEXER_NEXT_CP;
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
        }
    }
}

// https://html.spec.whatwg.org/#decimal-character-reference-state
static CrawlerLexerResult handle_decimal_character_reference_state(struct CrawlerInternalParserContext* parser, int cp) {
    switch(cp) {
    case 0x003B: // SEMICOLON (;)
        switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
        
        return CRAWLER_LEXER_NEXT_CP;
    default:
        if (is_ascii_digit(cp)) {
            if (parser->lexer.character_reference_code > UINT32_MAX / 10)
                parser->lexer.character_reference_code = UINT32_MAX;
            else
                parser->lexer.character_reference_code *= 10;

            add_to_value_safe(&parser->lexer.character_reference_code, cp-0x0030);
            return CRAWLER_LEXER_NEXT_CP;
        } else {
            crawler_parser_register_error(parser, CRAWLER_ERROR_MISSING_SEMICOLON_AFTER_CHARACTER_REFERENCE);
            switch_state(parser, CRAWLER_LEXER_STATE_NUMERIC_CHARACTER_REFERENCE_END);
            stream_reconsume(parser);
            return CRAWLER_LEXER_NEXT_CP;
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

    unsigned int crc = parser->lexer.character_reference_code;
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
    if (!temporary_append(parser, crc))
        return CRAWLER_LEXER_FAILURE;
    switch_to_return_state(parser);
    return flush_code_points_consumed_as_a_character_reference(parser);
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
    handle_self_closing_start_tag_state,
    handle_bogus_comment_state,
    handle_markup_declaration_open_state,
    handle_comment_start_state,
    handle_comment_start_dash_state,
    handle_comment_state,
    handle_comment_less_than_sign_state,
    handle_comment_less_than_sign_bang_state,
    handle_comment_less_than_sign_bang_dash_state,
    handle_comment_less_than_sign_bang_dash_dash_state,
    handle_comment_end_dash_state,
    handle_comment_end_state,
    handle_comment_end_bang_state,
    handle_doctype_state,
    handle_before_doctype_name_state,
    handle_doctype_name_state,
    handle_after_doctype_name_state,
    handle_after_doctype_public_keyword_state,
    handle_before_doctype_public_identifier_state,
    handle_doctype_public_identifier_double_quoted_state,
    handle_doctype_public_identifier_single_quoted_state,
    handle_after_doctype_public_identifier_state,
    handle_between_doctype_public_and_system_identifiers_state,
    handle_after_doctype_system_keyword_state,
    handle_before_doctype_system_identifier_state,
    handle_doctype_system_identifier_double_quoted_state,
    handle_doctype_system_identifier_single_quoted_state,
    handle_after_doctype_system_identifier_state,
    handle_bogus_doctype_state,
    handle_cdata_section_state,
    handle_cdata_section_bracket_state,
    handle_cdata_section_end_state,
    handle_processing_instruction_open_state,
    handle_processing_instruction_target_state,
    handle_after_processing_instruction_target_state,
    handle_processing_instruction_data_state,
    handle_processing_instruction_questionable_state,
    handle_character_reference_state,
    handle_named_character_reference_state,
    handle_ambiguous_ampersand_state,
    handle_numeric_character_reference_state,
    handle_hexadecimal_character_reference_start_state,
    handle_hexadecimal_character_reference_state,
    handle_decimal_character_reference_state,
    handle_numeric_character_reference_end_state
};

void crawler_lexer_init(struct CrawlerInternalLexerContext* lexer) {
    lexer->current_state = CRAWLER_LEXER_STATE_DATA;
    crawler_string_init(&lexer->temporary_buffer);
    lexer->start_tag_emitted = false;
    crawler_string_init(&lexer->last_emitted_start_tag_name);
    lexer->current_attribute_node = NULL;
    lexer->named_ref = NULL;
}

bool crawler_lexer_create(CrawlerLexerContext* lexer) {
    if (!crawler_string_create(&lexer->temporary_buffer, 4))
        return false;
    if (!crawler_string_create(&lexer->last_emitted_start_tag_name, 4))
        return false;
    if (!crawler_named_reference_create(lexer))
        return false;
    return true;
}

void crawler_lexer_destroy(CrawlerLexerContext* lexer) {
    crawler_string_destroy(&lexer->temporary_buffer);
    crawler_string_destroy(&lexer->last_emitted_start_tag_name);
    crawler_named_reference_destroy(lexer);
}

CrawlerLexerResult crawler_lexer_gen_token(struct CrawlerInternalParserContext* parser) {
    CrawlerUTF8Stream* is = &parser->is;

    CrawlerLexerResult step_result;
    do {
        if (crawler_stream_get(parser) == CRAWLER_STREAM_MISSING_ELEMENT)
            return CRAWLER_LEXER_MISSING_CP;
        step_result = kCrawlerHandlerDispatchTable[parser->lexer.current_state](parser, is->current_code_point);
    } while (step_result == CRAWLER_LEXER_NEXT_CP);

    if (CRAWLER_LEXER_SUCCESS) 
        stream_commit(parser);
    return step_result;
}