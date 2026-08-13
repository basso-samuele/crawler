#include "crawler.h"

CrawlerParserResult crawler_parse_buffer(CrawlerBuffer* buffer) {
    CrawlerParserContext parser;
    crawler_parser_init(&parser);

    crawler_parser_bind_buffer(&parser, buffer);

    while (1) {
        // Generate a token using the lexer.
        CrawlerLexerResult lr = crawler_lexer_gen_token(&parser);
        if (lr == CRAWLER_LEXER_MISSING_CP)
            return CRAWLER_PARSER_MISSING_DATA;

        CrawlerToken* token = &parser.current_token;
        crawler_debug("Generated token. Type: %d.\n", parser.current_token.type);

        // Freeing resources and simulating the transfer of ownership to the tree builder once a token is generated.
        if (token->type == CRAWLER_TOKEN_EOF) {
            crawler_token_destroy(token);
            return CRAWLER_PARSER_SUCCESS;
        } else {
            crawler_token_destroy(token);
        }
    }
}