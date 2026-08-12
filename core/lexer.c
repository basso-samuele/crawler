#include "lexer.h"
#include "parser.h"
#include "stream.h"

#include <stdbool.h>
#include <assert.h>

static void crawler_lexer_emit_prepare_multichar(struct CrawlerInternalParserContext* parser) {
    crawler_string_create(&parser->current_token.data.str, 1);
    parser->current_token.type = CRAWLER_TOKEN_CHARACTER;
}

static void crawler_lexer_emit_character(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_string_append(&parser->current_token.data.str, cp);
}

static void crawler_lexer_emit_current_character(struct CrawlerInternalParserContext* parser) {
    crawler_string_create(&parser->current_token.data.str, 1);
    crawler_string_append(&parser->current_token.data.str, parser->is.current_code_point);
    parser->current_token.type = CRAWLER_TOKEN_CHARACTER;
}

static void crawler_lexer_empty_string(CrawlerString* string) {
    crawler_string_create(string, 8);
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
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
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
        crawler_lexer_emit_prepare_multichar(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
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
        crawler_lexer_emit_prepare_multichar(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
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
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
        crawler_lexer_emit_current_character(parser);
        return CRAWLER_LEXER_SUCCESS;
    }
}

static CrawlerLexerResult handle_plaintext_state(struct CrawlerInternalParserContext* parser, int cp) {
    crawler_stream_commit(parser);
    switch(cp) {
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_total_offset);
        crawler_lexer_emit_prepare_multichar(parser);
        crawler_lexer_emit_character(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1:
        parser->current_token.type = CRAWLER_TOKEN_EOF;
        return CRAWLER_LEXER_SUCCESS;
    default:
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
        crawler_lexer_empty_string(&parser->lexer.temporary_buffer);
        parser->lexer.current_state = CRAWLER_LEXER_STATE_PROCESSING_INSTRUCTION_OPEN;
        crawler_stream_commit(parser);
        return CRAWLER_LEXER_NEXT_CP;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_BEFORE_TAG_NAME, cp, parser->is.current_total_offset);
        crawler_lexer_emit_prepare_multichar(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        // Return to state data will emit an EOF token if presented with a -1 codepoint. Reconsuming.
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        if (crawler_lexer_is_ascii_alpha(cp)) { // ASCII alpha
            // Create a new start tag token, set its tag name to the empty string.
            parser->current_token.type = CRAWLER_TOKEN_START_TAG;
            crawler_lexer_empty_string(&parser->current_token.data.start_tag.name);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_TAG_NAME;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else { // Anything else
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME, cp, parser->is.current_total_offset);
            crawler_lexer_emit_prepare_multichar(parser);
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
        crawler_lexer_emit_prepare_multichar(parser);
        crawler_lexer_emit_character(parser, 0x003C);
        crawler_lexer_emit_character(parser, 0x002F);
        // Return to state data will emit an EOF token if presented with a -1 codepoint. Reconsuming.
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        crawler_stream_reset(parser);
        return CRAWLER_LEXER_ERROR;
    default:
        if (crawler_lexer_is_ascii_alpha(cp)) { // ASCII alpha
            // Create a new end tag token, set its tag name to the empty string.
            parser->current_token.type = CRAWLER_TOKEN_END_TAG;
            crawler_lexer_empty_string(&parser->current_token.data.end_tag);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_TAG_NAME;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_NEXT_CP;
        } else { // Anything else
            crawler_parser_register_error(parser, CRAWLER_ERROR_INVALID_FIRST_CHARACTER_OF_TAG_NAME, cp, parser->is.current_total_offset);
            // Create a comment token whose data is the empty string.
            parser->current_token.type = CRAWLER_TOKEN_COMMENT;
            crawler_lexer_empty_string(&parser->current_token.data.str);
            parser->lexer.current_state = CRAWLER_LEXER_STATE_BOGUS_COMMENT;
            crawler_stream_reset(parser);
            return CRAWLER_LEXER_ERROR;
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
        // A token is considered emitted when CRAWLER_LEXER_SUCCESS is returned.
        // All data relative to the token being emitted is already in parser->current_token.
        parser->lexer.current_state = CRAWLER_LEXER_STATE_DATA;
        return CRAWLER_LEXER_SUCCESS;
    case 0x0000: // NULL
        crawler_parser_register_error(parser, CRAWLER_ERROR_UNEXPECTED_NULL_CHARACTER, cp, parser->is.current_code_point);
        crawler_lexer_append_tag_name(parser, 0xFFFD);
        return CRAWLER_LEXER_ERROR;
    case -1: // EOF
        crawler_parser_register_error(parser, CRAWLER_ERROR_EOF_IN_TAG, cp, parser->is.current_code_point);
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

typedef CrawlerLexerResult(*CrawlerLexerHandler)(struct CrawlerInternalParserContext*, int);
static CrawlerLexerHandler kCrawlerHandlerDispatchTable[] = {
    handle_data_state,
    handle_rcdata_state,
    handle_rawtext_state,
    handle_script_data_state,
    handle_plaintext_state,
    handle_tag_open_state,
    handle_end_tag_open_state,
    handle_tag_name_state
};

void crawler_lexer_init(struct CrawlerInternalParserContext* parser) {
    CrawlerLexerContext* lexer = &parser->lexer;
    // The state machine must start in the data state.
    lexer->current_state = CRAWLER_LEXER_STATE_DATA;
    crawler_string_init(&lexer->temporary_buffer);
}

CrawlerLexerResult crawler_lexer_gen_token(struct CrawlerInternalParserContext* parser) {
    CrawlerUTF8Stream* is = &parser->is;
    CrawlerStreamResult sr = crawler_stream_peek(parser);
    switch(sr) {
        case CRAWLER_STREAM_MISSING_ELEMENT:
            return CRAWLER_LEXER_MISSING_CP;
        case CRAWLER_STREAM_ERROR:
        case CRAWLER_STREAM_SUCCESS:
    }
    return kCrawlerHandlerDispatchTable[parser->lexer.current_state](parser, is->current_code_point);
}