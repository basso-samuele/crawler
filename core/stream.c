#include "stream.h"
#include "error.h"
#include "parser.h"

#include <stdbool.h>
#include <assert.h>
#include <string.h>

// TODO: switch to a single return statement and inline to see if it changes execution time.
static bool crawler_verify_codepoint_validity(size_t length, int cp) {
    if (cp > 0x10FFFF) return false;
    if (cp >= 0xD800 && cp <= 0xDFFF) return false;
    if (length == 2 && cp < 0x80) return false;
    if (length == 3 && cp < 0x800) return false;
    if (length == 4 && cp < 0x10000) return false;
    return true;
}

// https://infra.spec.whatwg.org/#surrogate
static bool crawler_is_surrogate(int cp) {
    return ((cp >= 0xD800) && (cp <= 0xDBFF)) ||
           ((cp >= 0xDC00) && (cp <= 0xDFFF));
}

// https://infra.spec.whatwg.org/#noncharacter
static bool crawler_is_noncharacter(int cp) {
    return ((cp >= 0xFDD0) && (cp <= 0xFDEF)) ||
           ((cp <= 0x10FFFF) && ((cp & 0xFFFF) >= 0xFFFE));
}

// https://infra.spec.whatwg.org/#control
static bool crawler_is_control_other_than_ascii_and_null(int cp) {
    return ((cp >= 0x007F) && (cp <= 0x009F)) &&
           !((cp == 0x0009) || (cp == 0x000A) || (cp == 0x000C) || (cp == 0x000D) || (cp == 0x0020));
}

// https://html.spec.whatwg.org/#parse-errors
static void crawler_iterator_sanitize(struct CrawlerInternalParserContext* parser, int cp) {
    if (crawler_is_surrogate(cp)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_SURROGATE_IN_INPUT_STREAM);
    }

    if (crawler_is_noncharacter(cp)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_NONCHARACTER_IN_INPUT_STREAM);
    }

    if (crawler_is_control_other_than_ascii_and_null(cp)) {
        crawler_parser_register_error(parser, CRAWLER_ERROR_CONTROL_CHARACTER_IN_INPUT_STREAM);
    }
}

static CrawlerStreamResult stream_peek_one_byte(const CrawlerUTF8Stream* is, const CrawlerBuffer* buffer, size_t offset, int* cp, size_t* cp_size) {
    *cp = (int)buffer->base[offset];

    if (*cp != 0x000D) {
        *cp_size = 1;
        return CRAWLER_STREAM_SUCCESS;
    }

    *cp = 0x000A;
    if (buffer->size - offset >= 2) {
        int ncp = (int)buffer->base[offset+1];
        *cp_size = ncp == 0x000A ? 2 : 1;
        return CRAWLER_STREAM_SUCCESS;
    } if (buffer->eof) {
        *cp_size = 1;
        return CRAWLER_STREAM_SUCCESS;
    } else {
        return CRAWLER_STREAM_MISSING_ELEMENT;
    }
}

static CrawlerStreamResult stream_peek_two_byte(const CrawlerUTF8Stream* is, const CrawlerBuffer* buffer, size_t offset, int* cp, size_t* cp_size) {
    if (buffer->size - offset >= 2) {
        unsigned char b1 = buffer->base[offset];
        unsigned char b2 = buffer->base[offset+1];

        if ((b2&0b11000000)!=0b10000000)
            return CRAWLER_STREAM_ERROR;

        *cp = (int)(((b1&0b00011111)<<6)|(b2&0b00111111));
        *cp_size = 2;

        if (!crawler_verify_codepoint_validity(*cp_size, *cp))
            return CRAWLER_STREAM_ERROR;

        return CRAWLER_STREAM_SUCCESS;
    } else {
        return CRAWLER_STREAM_MISSING_ELEMENT;
    }
}

static CrawlerStreamResult stream_peek_three_byte(const CrawlerUTF8Stream* is, const CrawlerBuffer* buffer, size_t offset, int* cp, size_t* cp_size) {
    if (buffer->size - offset >= 3) {
        unsigned char b1 = buffer->base[offset];
        unsigned char b2 = buffer->base[offset+1];
        unsigned char b3 = buffer->base[offset+2];

        if ((b2&0b11000000)!=0b10000000 || (b3&0b11000000)!=0b10000000)
            return CRAWLER_STREAM_ERROR;

        *cp = (int)(((b1&0b00001111)<<12)|((b2&0b00111111)<<6)|(b3&0b00111111));
        *cp_size = 3;

        if (!crawler_verify_codepoint_validity(*cp_size, *cp))
            return CRAWLER_STREAM_ERROR;

        return CRAWLER_STREAM_SUCCESS;
    } else {
        return CRAWLER_STREAM_MISSING_ELEMENT;
    }
}

static CrawlerStreamResult stream_peek_four_byte(const CrawlerUTF8Stream* is, const CrawlerBuffer* buffer, size_t offset, int* cp, size_t* cp_size) {
    if (buffer->size - offset >= 4) {
        unsigned char b1 = buffer->base[offset];
        unsigned char b2 = buffer->base[offset+1];
        unsigned char b3 = buffer->base[offset+2];
        unsigned char b4 = buffer->base[offset+3];

        if ((b2&0b11000000)!=0b10000000 || (b3&0b11000000)!=0b10000000 || (b4&0b11000000)!=0b10000000)
            return CRAWLER_STREAM_ERROR;

        *cp = (int)(((b1&0b00000111)<<18)|((b2&0b00111111)<<12)|((b3&0b00111111)<<6)|(b4&0b00111111));
        *cp_size = 4;

        if (!crawler_verify_codepoint_validity(*cp_size, *cp))
            return CRAWLER_STREAM_ERROR;

        return CRAWLER_STREAM_SUCCESS;
    } else {
        return CRAWLER_STREAM_MISSING_ELEMENT;
    }
}

// TODO: when peeking and the getting the errors signaled by sanitize may be registered more than once.
// When the lexer is completed and stream simplified it may be easy to solve.
static CrawlerStreamResult stream_peek(struct CrawlerInternalParserContext* parser, int* cp, size_t* cp_size) {
    const CrawlerUTF8Stream* is = &parser->is;
    const CrawlerBuffer* buffer = is->buffer;
    const size_t offset = is->head + is->offset;

    if (buffer->size > offset) {
        unsigned char b1 = buffer->base[offset];
        if ((b1&0b10000000)==0b0) {
            CrawlerStreamResult r = stream_peek_one_byte(is, buffer, offset, cp, cp_size);
            if (r == CRAWLER_STREAM_SUCCESS)
                crawler_iterator_sanitize(parser, *cp);
            return r;
        } else if ((b1&0b11100000)==0b11000000) {
            CrawlerStreamResult r = stream_peek_two_byte(is, buffer, offset, cp, cp_size);
            if (r == CRAWLER_STREAM_SUCCESS)
                crawler_iterator_sanitize(parser, *cp);
            return r;
        } else if ((b1&0b11110000)==0b11100000) {
            CrawlerStreamResult r = stream_peek_three_byte(is, buffer, offset, cp, cp_size);
            if (r == CRAWLER_STREAM_SUCCESS)
                crawler_iterator_sanitize(parser, *cp);
            return r;
        } else if ((b1&0b11111000)==0b11110000) {
            CrawlerStreamResult r = stream_peek_four_byte(is, buffer, offset, cp, cp_size);
            if (r == CRAWLER_STREAM_SUCCESS)
                crawler_iterator_sanitize(parser, *cp);
            return r;
        } else {
            return CRAWLER_STREAM_ERROR;
        }
    } else if (buffer->eof) {
        *cp = -1;
        return CRAWLER_STREAM_SUCCESS;
    }

    return CRAWLER_STREAM_MISSING_ELEMENT;
}

CrawlerStreamResult crawler_stream_peek(struct CrawlerInternalParserContext* parser, int* cp) {
    int maybe_cp;
    size_t maybe_cp_size;
    CrawlerStreamResult peek_res = stream_peek(parser, &maybe_cp, &maybe_cp_size);
    if (peek_res == CRAWLER_STREAM_SUCCESS)
        *cp = maybe_cp;
    return peek_res;
}

CrawlerStreamResult crawler_stream_get(struct CrawlerInternalParserContext* parser) {
    if (parser->is.reconsume) {
        parser->is.reconsume = false;
        return CRAWLER_STREAM_SUCCESS;
    }

    int maybe_cp;
    size_t maybe_cp_size;
    CrawlerStreamResult peek_res = stream_peek(parser, &maybe_cp, &maybe_cp_size);
    if (peek_res == CRAWLER_STREAM_SUCCESS) {
        parser->is.current_code_point = maybe_cp;
        parser->is.current_code_point_offset = parser->is.head + parser->is.offset;
        parser->is.offset += maybe_cp_size;
    }
    return peek_res;
}

void crawler_stream_reconsume(CrawlerUTF8Stream* stream) {
    stream->reconsume = true;
}

void crawler_stream_init(CrawlerUTF8Stream* stream) {
    memset(stream, 0, sizeof *stream);
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
        (stream->current_code_point_offset + length <= stream->buffer->size) &&
        (case_sensitive ? !strncmp(stream->buffer->base + stream->current_code_point_offset, prefix, length)
                        : !strncasecmp(stream->buffer->base + stream->current_code_point_offset, prefix, length));
    if (matched) {
        stream->current_code_point_offset += length;
        stream->head = stream->current_code_point_offset;
        stream->offset = 0;
        return true;
    } else {
        return false;
    }
}