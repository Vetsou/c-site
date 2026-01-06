#ifndef _RT_HTTP_PARSER_H_
#define _RT_HTTP_PARSER_H_

#include <stddef.h>
#include <stdint.h>

/**
 * Set maximum HTTP request size for safety reasons.
 * Default is 4KB.
 */
#ifndef HTTP_MAX_REQ_SIZE
#define HTTP_MAX_REQ_SIZE (4*1024)
#endif // HTTP_MAX_REQ_SIZE

#define HTTP_MAX_HEADER_LEN 64

/**
 * RT only supports status codes it may emit.
 */
typedef enum {
    // 200s codes
    RT_OK = 200,

    // 400s codes
    RT_BAD_REQUEST = 400,
    RT_FORBIDDEN = 403,
    RT_NOT_FOUND = 404,

    // 500s codes
    RT_INTERNAL_SERVER_ERROR = 500
} rt_http_status;


/**
 * RT doesn't need support for all HTTP methods.
 * This enum specifies the supported methods.
 */
typedef enum {
    RT_UNDEFINED_METHOD = -1,

    RT_GET = 0
} rt_http_method;


/**
 * RT only accepts HTTP/1.0 and HTTP/1.1
 */
typedef enum {
    RT_UNDEFINED_VERSION = -1,

    RT_HTTP_10 = 0,
    RT_HTTP_11
} rt_http_version;


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
} rt_header_data;


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
    rt_header_data headers[HTTP_MAX_HEADER_LEN];
} rt_req_data;


void rt_init_http_parser(
    rt_http_req_parser *p,
    char *start,
    const char *end);

rt_parse_result rt_parse_req(
    rt_http_req_parser *p,
    rt_req_data *req_data);

#endif // _RT_HTTP_PARSER_H_