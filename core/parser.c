#include "parser.h"
#include "stream.h"
#include "utils.h"
#include "lexer.h"
#include "token.h"

void crawler_parser_init(CrawlerParserContext* parser) {
    crawler_stream_init(parser);
    crawler_lexer_init(parser);
    crawler_token_init(&parser->current_token);
}

void crawler_parser_bind_buffer(CrawlerParserContext* parser, CrawlerBuffer* buffer) {
    parser->is.buffer = buffer;
    crawler_stream_init(parser);
}

void crawler_parser_register_error(CrawlerParserContext* parser, CrawlerParseErrorType error_code, int code_point, size_t offset) {
    crawler_debug("Error raised. Code: %d. Caused by code point: %d. Found at offset: %d.\n", (int)error_code, code_point, offset);
}