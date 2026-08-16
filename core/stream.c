#include "stream.h"
#include "error.h"
#include "parser.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h>

static bool crawler_verify_codepoint_validity(size_t length, int cp) {
    // TODO: switch to a single return statement and inline to see if it changes execution time.
    if (cp > 0x10FFFF) return false;
    if (cp >= 0xD800 && cp <= 0xDFFF) return false;
    if (length == 2 && cp < 0x80) return false;
    if (length == 3 && cp < 0x800) return false;
    if (length == 4 && cp < 0x10000) return false;
    return true;
}

static bool crawler_is_surrogate(int cp) {
    // https://infra.spec.whatwg.org/#surrogate
    return ((cp >= 0xD800) && (cp <= 0xDBFF)) ||
           ((cp >= 0xDC00) && (cp <= 0xDFFF));
}

static bool crawler_is_noncharacter(int cp) {
    // https://infra.spec.whatwg.org/#noncharacter
    return ((cp >= 0xFDD0) && (cp <= 0xFDEF)) ||
           ((cp <= 0x10FFFF) && ((cp & 0xFFFF) >= 0xFFFE));
}

static bool crawler_is_control_other_than_ascii_and_null(int cp) {
    // https://infra.spec.whatwg.org/#control
    return ((cp >= 0x007F) && (cp <= 0x009F)) &&
           !((cp == 0x0009) || (cp == 0x000A) || (cp == 0x000C) || (cp == 0x000D) || (cp == 0x0020));
}

static void crawler_iterator_sanitize(struct CrawlerInternalParserContext* parser) {
    // https://html.spec.whatwg.org/#parse-errors
    // ...parsed as-is and usually, where parsing rules don't apply any additional restrictions, make their way into the DOM...

    // Any occurrences of surrogates are surrogate-in-input-stream parse errors.
    if (crawler_is_surrogate(parser->is.current_code_point)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_SURROGATE_IN_INPUT_STREAM);
    }

    // Any occurrences of noncharacters are noncharacter-in-input-stream parse.
    if (crawler_is_noncharacter(parser->is.current_code_point)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_NONCHARACTER_IN_INPUT_STREAM);
    }

    // Any occurrences of controls other than ASCII whitespace and U+0000 NULL characters are control-character-in-input-stream parse errors.
    if (crawler_is_control_other_than_ascii_and_null(parser->is.current_code_point)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_CONTROL_CHARACTER_IN_INPUT_STREAM);
    }
}

CrawlerStreamResult crawler_stream_peek(struct CrawlerInternalParserContext* parser) {
    CrawlerBuffer* buffer = parser->is.buffer;
    size_t total_offset = parser->is.head + parser->is.offset;
    if (buffer->size > total_offset) {
        size_t codepoint_length = 1;
        unsigned char b1 = buffer->base[total_offset];
        if ((b1&0b10000000)==0b0) {
            int cp = (int)b1;
            parser->is.current_total_offset = total_offset;
            // To normalize newlines in a string, replace every U+000D CR U+000A LF
            // code point pair with a single U+000A LF code point, and then replace
            // every remaining U+000D CR code point with a U+000A LF code point.
            if (cp == 0x000D) {
                parser->is.current_code_point = 0x000A;
                if (buffer->size - total_offset >= 2) {
                    unsigned char b2 = buffer->base[total_offset+1];
                    if ((int)b2 == 0x000A) {
                        parser->is.offset += 2;
                    } else {
                        parser->is.offset += 1;
                    }
                }
                parser->is.offset += 1;
            } else {
                parser->is.current_code_point = cp;
                parser->is.offset += 1;
            }
            crawler_iterator_sanitize(parser);
            return CRAWLER_STREAM_SUCCESS;
        } else if ((b1&0b11100000)==0b11000000) {
            if (buffer->size - total_offset >= 2) {
                unsigned char b2 = buffer->base[total_offset+1];
                if ((b2&0b11000000)!=0b10000000) return CRAWLER_STREAM_ERROR;
                codepoint_length = 2;
                int cp =
                    (int)(((b1&0b00011111)<<6)|(b2&0b00111111));
                if (!crawler_verify_codepoint_validity(codepoint_length, cp))
                    return CRAWLER_STREAM_ERROR;
                parser->is.current_code_point = cp;
                parser->is.current_total_offset = total_offset;
                parser->is.offset += 2;
                crawler_iterator_sanitize(parser);
                return CRAWLER_STREAM_SUCCESS;
            } else {
                return CRAWLER_STREAM_MISSING_ELEMENT;
            }
        } else if ((b1&0b11110000)==0b11100000) {
            if (buffer->size - total_offset >= 3) {
                unsigned char b2 = buffer->base[total_offset+1];
                unsigned char b3 = buffer->base[total_offset+2];
                if ((b2&0b11000000)!=0b10000000) return CRAWLER_STREAM_ERROR;
                if ((b3&0b11000000)!=0b10000000) return CRAWLER_STREAM_ERROR;
                codepoint_length = 3;
                int cp =
                    (int)(((b1&0b00001111)<<12)|((b2&0b00111111)<<6)|(b3&0b00111111));
                if (!crawler_verify_codepoint_validity(codepoint_length, cp))
                    return CRAWLER_STREAM_ERROR;
                parser->is.current_code_point = cp;
                parser->is.current_total_offset = total_offset;
                parser->is.offset += 3;
                crawler_iterator_sanitize(parser);
                return CRAWLER_STREAM_SUCCESS;
            } else {
                return CRAWLER_STREAM_MISSING_ELEMENT;
            }
        } else if ((b1&0b11111000)==0b11110000) {
            if (buffer->size - total_offset >= 4) {
                unsigned char b2 = buffer->base[total_offset+1];
                unsigned char b3 = buffer->base[total_offset+2];
                unsigned char b4 = buffer->base[total_offset+3];
                if ((b2&0b11000000)!=0b10000000) return CRAWLER_STREAM_ERROR;
                if ((b3&0b11000000)!=0b10000000) return CRAWLER_STREAM_ERROR;
                if ((b4&0b11000000)!=0b10000000) return CRAWLER_STREAM_ERROR;
                codepoint_length = 4;
                int cp =
                    (int)(((b1&0b00000111)<<18)|((b2&0b00111111)<<12)|((b3&0b00111111)<<6)|(b4&0b00111111));
                if (!crawler_verify_codepoint_validity(codepoint_length, cp))
                    return CRAWLER_STREAM_ERROR;
                parser->is.current_code_point = cp;
                parser->is.current_total_offset = total_offset;
                parser->is.offset += 4;
                crawler_iterator_sanitize(parser);
                return CRAWLER_STREAM_SUCCESS;
            } else {
                return CRAWLER_STREAM_MISSING_ELEMENT;
            }
        } else {
            return CRAWLER_STREAM_ERROR;
        }
    } else if (buffer->eof) {
        parser->is.current_code_point = -1;
        return CRAWLER_STREAM_SUCCESS;
    }

    return CRAWLER_STREAM_MISSING_ELEMENT;
}

void crawler_stream_init(CrawlerUTF8Stream* stream) {
    stream->buffer = NULL;
    stream->current_code_point = 0;
    stream->current_total_offset = 0;
    stream->offset = 0;
    stream->head = 0;
}

void crawler_stream_commit(CrawlerUTF8Stream* stream) {
    stream->head = stream->head + stream->offset;
    stream->offset = 0;
}

void crawler_stream_reset(CrawlerUTF8Stream* stream) {
    stream->offset = 0;
}

bool crawler_stream_consume_match(CrawlerUTF8Stream* stream, const char* prefix, size_t length, bool case_sensitive) {
    bool matched =
        (stream->current_total_offset + length < stream->buffer->size) &&
        (case_sensitive ? !strncmp(stream->buffer->base + stream->current_total_offset, prefix, length)
                        : !strncasecmp(stream->buffer->base + stream->current_total_offset, prefix, length));
    if (matched) {
        stream->current_total_offset += length;
        stream->head = stream->current_total_offset;
        stream->offset = 0;
        return true;
    } else {
        return false;
    }
}