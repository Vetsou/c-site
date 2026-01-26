#ifndef _RT_HTTP_PARSER_H_
#define _RT_HTTP_PARSER_H_

#include <stddef.h>
#include <stdint.h>

#define REQ_MAX_HEADER_COUNT 64

typedef enum {
    RT_UNKNOWN_ERR = -1,
    RT_PARSE_OK = 0,
    RT_PARSE_ERR,

    RT_TOO_MANY_HEADERS,
    RT_INVALID_HEADER_FOLD,
    RT_INVALID_CHAR,
    RT_UNEXPECTED_EOF
} rt_parse_result;


typedef struct {
    const char *start;
    const char *end;

    size_t nread;
    char *curr;
} rt_http_req_parser;


typedef struct {
    const char *name;
    int32_t name_len;
    const char *value;
    int32_t value_len;
} rt_req_header_view;


typedef struct {
    // Request line data
    const char *method;
    int32_t method_len;
    const char *path;
    int32_t path_len;
    const char *version;
    int32_t version_len;

    // Headers
    int32_t header_count;
    rt_req_header_view headers[REQ_MAX_HEADER_COUNT];
} rt_req_data;


void rt_init_http_parser(
    rt_http_req_parser *p,
    char *start,
    const char *end);

rt_parse_result rt_parse_req(
    rt_http_req_parser *p,
    rt_req_data *req_data);

#endif // _RT_HTTP_PARSER_H_