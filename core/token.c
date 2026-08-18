#include "token.h"
#include "utils.h"

#include <string.h>

void crawler_token_init(CrawlerToken* token) {
    token->type = CRAWLER_TOKEN_TYPE_UNKNOWN;
    memset(&token->data, 0, sizeof token->data);
}

void crawler_token_destroy(CrawlerToken* token) {
    switch(token->type) {
    case CRAWLER_TOKEN_DOCTYPE:
        crawler_string_destroy(&token->data.doc_type.name);
        crawler_string_destroy(&token->data.doc_type.public_identifier);
        crawler_string_destroy(&token->data.doc_type.system_identifier);
        break;
    case CRAWLER_TOKEN_START_TAG:
        crawler_string_destroy(&token->data.start_tag.name);
        crawler_attribute_list_destroy(&token->data.start_tag.attributes);
        break;
    case CRAWLER_TOKEN_END_TAG:
        crawler_string_destroy(&token->data.end_tag);
        break;
    case CRAWLER_TOKEN_COMMENT:
    case CRAWLER_TOKEN_CHARACTER:
        crawler_string_destroy(&token->data.str);
        break;
    case CRAWLER_TOKEN_EOF:
    case CRAWLER_TOKEN_PROCESSING_INSTRUCTION:
        crawler_string_destroy(&token->data.str);
        break;
    default:
    }
    crawler_token_init(token);
}