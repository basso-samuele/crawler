#ifndef CRAWLER_H_
#define CRAWLER_H_

#include "attributes.h"
#include "buffer.h"
#include "error.h"
#include "lexer.h"
#include "memory.h"
#include "parser.h"
#include "stream.h"
#include "string_buffer.h"
#include "tags.h"
#include "token.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CRAWLER_PARSER_MISSING_DATA,
    CRAWLER_PARSER_SUCCESS
} CrawlerParserResult;

CrawlerParserResult crawler_parse_buffer(CrawlerBuffer* buffer);

#ifdef __cplusplus
}
#endif
#endif // CRAWLER_H_