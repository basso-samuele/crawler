#include "token.h"
#include "utils.h"

void crawler_token_init(CrawlerToken* token) {
    token->type = CRAWLER_TOKEN_TYPE_UNKNOWN;
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
        break;
    case CRAWLER_TOKEN_END_TAG:
        crawler_string_destroy(&token->data.end_tag);
        break;
    case CRAWLER_TOKEN_COMMENT:
    case CRAWLER_TOKEN_CHARACTER:
        crawler_string_destroy(&token->data.str);
        break;
    case CRAWLER_TOKEN_EOF:
    case CRAWLER_TOKEN_TAG:
    case CRAWLER_TOKEN_PROCESSING_INSTRUCTION:
    default:
    }
    token->type = CRAWLER_TOKEN_TYPE_UNKNOWN;
}

void crawler_token_clone(CrawlerToken* destination, CrawlerToken* source) {
    switch(source->type) {
    case CRAWLER_TOKEN_DOCTYPE:
        crawler_string_clone(&destination->data.doc_type.name             , &source->data.doc_type.name             );
        crawler_string_clone(&destination->data.doc_type.public_identifier, &source->data.doc_type.public_identifier);
        crawler_string_clone(&destination->data.doc_type.system_identifier, &source->data.doc_type.system_identifier);
        destination->data.doc_type.has_public_identifier = source->data.doc_type.has_public_identifier;
        destination->data.doc_type.has_system_identifier = source->data.doc_type.has_system_identifier;
        destination->data.doc_type.force_quirks =          source->data.doc_type.force_quirks;
        break;
    case CRAWLER_TOKEN_START_TAG:
        crawler_string_clone(&destination->data.start_tag.name, &source->data.start_tag.name);
        destination->data.start_tag.is_self_closing = source->data.start_tag.is_self_closing;
        crawler_debug("Not cloning attributes.");
        destination->data.start_tag.attributes      = source->data.start_tag.attributes;
        break;
    case CRAWLER_TOKEN_END_TAG:
        crawler_string_clone(&destination->data.end_tag, &source->data.end_tag);
        break;
    case CRAWLER_TOKEN_COMMENT:
    case CRAWLER_TOKEN_CHARACTER:
        crawler_string_clone(&destination->data.str, &source->data.str);
        break;
    case CRAWLER_TOKEN_EOF:
    case CRAWLER_TOKEN_TAG:
    case CRAWLER_TOKEN_PROCESSING_INSTRUCTION:
    default:
    }
    destination->type = source->type;
}