#ifndef CRAWLER_PARSER_H_
#define CRAWLER_PARSER_H_

#include <stddef.h>

#include "stream.h"
#include "error.h"
#include "token.h"
#include "lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Represents the complete state, every function takes a pointer to this structure and
// modifies the necessary fields.
typedef struct CrawlerInternalParserContext {
    struct CrawlerInternalUTF8Stream is;
    struct CrawlerInternalLexerContext lexer;
    struct CrawlerInternalToken current_token;
} CrawlerParserContext;

void crawler_parser_init(CrawlerParserContext* parser);
void crawler_parser_bind_buffer(CrawlerParserContext* parser, CrawlerBuffer* buffer);
void crawler_parser_register_error(CrawlerParserContext* parser, CrawlerParseErrorType error_code, int code_point, size_t offset);

#ifdef __cplusplus
}
#endif
#endif