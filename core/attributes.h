#ifndef CRAWLER_ATTRIBUTES_H_
#define CRAWLER_ATTRIBUTES_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Representing a single attribute with name and value.
 * 
 * Since heap allocation can be expensive, I would like to substitute name and value with structures supporting SSO.
 */
typedef struct {
    const int* name;
    const int* value;
} CrawlerAttribute;

#ifdef __cplusplus
}
#endif
#endif