#include "parser.h"
#include "stream.h"
#include "utils.h"
#include "lexer.h"
#include "token.h"

void crawler_parser_init(CrawlerParserContext* parser) {
    crawler_stream_init(&parser->is);
    crawler_lexer_init(&parser->lexer);
    crawler_token_init(&parser->current_token);
}

void crawler_parser_bind_buffer(CrawlerParserContext* parser, CrawlerBuffer* buffer) {
    crawler_stream_init(&parser->is);
    parser->is.buffer = buffer;
}

void crawler_parser_register_error(CrawlerParserContext* parser, CrawlerParseErrorType error_code) {
    crawler_debug(
        "Error raised. Code: %d. Caused by code point: %d. Found at offset: %d.\n",
        error_code, parser->is.current_code_point,
        parser->is.current_code_point_offset
    );
}