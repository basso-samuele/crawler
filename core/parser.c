#include "parser.h"
#include "stream.h"
#include "utils.h"
#include "lexer.h"

static void crawler_parser_token_init(CrawlerParserContext* parser) {
    CrawlerToken* token = &parser->current_token;
    token->type = CRAWLER_TOKEN_TYPE_UNKNOWN;
}

void crawler_parser_init(CrawlerParserContext* parser) {
    crawler_stream_init(parser);
    crawler_lexer_init(parser);
    crawler_parser_token_init(parser);
}

void crawler_parser_bind_buffer(CrawlerParserContext* parser, CrawlerBuffer* buffer) {
    parser->is.buffer = buffer;
    crawler_stream_init(parser);
}

void crawler_parser_register_error(CrawlerParserContext* parser, CrawlerParseErrorType error_code, int code_point, size_t offset) {
    crawler_debug("Error raised. Code: %d. Caused by code point: %d. Found at offset: %d.\n", (int)error_code, code_point, offset);
}

CrawlerParserResult crawler_parse(CrawlerBuffer* buffer) {
    CrawlerParserContext parser;
    crawler_parser_bind_buffer(&parser, buffer);
    crawler_lexer_init(&parser);

    while (1) {
        // Generate a token using the lexer.
        CrawlerLexerResult lr;
        do {
            lr = crawler_lexer_gen_token(&parser);
            if (lr == CRAWLER_LEXER_MISSING_CP)
                return CRAWLER_PARSER_MISSING_DATA;
        } while (lr == CRAWLER_LEXER_NEXT_CP);

        CrawlerToken* token = &parser.current_token;
        crawler_debug("Generated token. Type: %d.\n", parser.current_token.type);

        if (token->type == CRAWLER_TOKEN_EOF)
            return CRAWLER_PARSER_SUCCESS;

        // Consume the generated token using the tree builder.
    }
}