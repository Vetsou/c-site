#include "rt_http_parser.h"

#include <stdbool.h>
#include <string.h>
#include <ctype.h>

// --------------------------------------------------
// PRIVATE
// --------------------------------------------------
#define IS_PRINTABLE_ASCII(c) ((unsigned char)(c) >= 0x20 && (unsigned char)(c) <= 0x7E)

#define CHECK_EOF()               \
    if (p->curr == p->end) {      \
        return RT_UNEXPECTED_EOF; \
    }

#define EXPECT_CHAR_NO_CHECK(ch) \
    if (*p->curr++ != ch) {      \
        return RT_INVALID_CHAR;  \
    }

#define EXPECT_CHAR(ch)       \
    CHECK_EOF();              \
    EXPECT_CHAR_NO_CHECK(ch);

static bool is_tok_char(
    char c
) {
    if (isalnum((unsigned char)c)) return true;
    if (c == '/' || c == '_' || c == '-') return true;
    return false;
}

static rt_parse_result parse_token(
    rt_http_req_parser *p,
    const char **token,
    int32_t *token_len,
    char delim
) {
    const char *tok_start = p->curr;

    while (true) {
        if (*p->curr == delim) break;
        if (!is_tok_char(*p->curr)) return RT_INVALID_CHAR;

        p->curr++;
        CHECK_EOF();
    }

    *token = tok_start;
    *token_len = (int32_t)(p->curr - tok_start);
    return RT_PARSE_OK;
}

static rt_parse_result parse_token_to_eol(
    rt_http_req_parser *p,
    const char **token,
    int32_t *token_len
) {
    const char *tok_start = p->curr;
    const char *tok_end = p->curr;

    while (true) {
        CHECK_EOF();
        char c = *p->curr;

        // Check \r or \n
        if (c == '\015' || c == '\012') break;

        if ((c < '\040' && c != '\011') || c == '\177') {
            return RT_INVALID_CHAR;
        }

        p->curr++;
        tok_end = p->curr;
    }

    while (tok_end > tok_start) {
        unsigned char c = (unsigned char)*(tok_end - 1);
        if (c != '\040' && c != '\011') break;
        tok_end--;
    }

    *token = tok_start;
    *token_len = (int32_t)(tok_end - tok_start);

    if (*p->curr == '\015') {
        p->curr++;
        EXPECT_CHAR('\012');
    } else if (*p->curr == '\012') {
        p->curr++;
    } else {
        return RT_INVALID_CHAR;
    }

    return RT_PARSE_OK;
}

static rt_parse_result parse_path(
    rt_http_req_parser *p,
    const char **buf,
    int32_t *buf_len
) {
    const char *tok_start = p->curr;

    while (true) {
        if (*p->curr == ' ') break;
        
        if (!IS_PRINTABLE_ASCII(*p->curr)) {
            return RT_INVALID_CHAR;
        }

        p->curr++;
        CHECK_EOF();
    }

    *buf = tok_start;
    *buf_len = (int32_t)(p->curr - tok_start);
    return RT_PARSE_OK;
}

static rt_parse_result parse_version(
    rt_http_req_parser *p,
    const char **buf,
    int32_t *buf_len
) {
    if (p->end - p->curr < 9) {
        return RT_UNEXPECTED_EOF;
    }

    const char *tok_start = p->curr;

    EXPECT_CHAR_NO_CHECK('H');
    EXPECT_CHAR_NO_CHECK('T');
    EXPECT_CHAR_NO_CHECK('T');
    EXPECT_CHAR_NO_CHECK('P');
    EXPECT_CHAR_NO_CHECK('/');
    EXPECT_CHAR_NO_CHECK('1');
    EXPECT_CHAR_NO_CHECK('.');

    // We accept only HTTP/1.0 and HTTP/1.1
    CHECK_EOF();
    if (*p->curr != '0' && *p->curr != '1') {
        return RT_INVALID_CHAR;
    }
    p->curr++;

    *buf = tok_start;
    *buf_len = (int32_t)(p->curr - tok_start);

    return RT_PARSE_OK;
}

static rt_parse_result parse_headers(
    rt_http_req_parser *p,
    rt_req_header_view *headers,
    int32_t *header_count
) {
    int32_t num_headers = 0;

    for (;; num_headers++) {
        CHECK_EOF();

        // Break on \r\n or \n
        if (*p->curr == '\015') {
            p->curr++;
            EXPECT_CHAR('\012');
            break;
        } else if (*p->curr == '\012') {
            p->curr++;
            break;
        }

        // RT rejects folded headers
        if (*p->curr == ' ' || *p->curr == '\t') {
            return RT_INVALID_HEADER_FOLD;
        }

        if (num_headers == MAX_REQ_HEADER_LEN) {
            return RT_TOO_MANY_HEADERS;
        }

        // Parse header name
        rt_parse_result ret = parse_token(p, &headers[num_headers].name, &headers[num_headers].name_len, ':');
        if (ret != RT_PARSE_OK) return ret;

        if (headers[num_headers].name_len == 0) {
            return RT_PARSE_ERR;
        }

        // Skip whitespace
        p->curr++;
        while (true) {
            CHECK_EOF();
            if (*p->curr != ' ' && *p->curr != '\t') break;
            p->curr++;
        }

        // Parse header value
        ret = parse_token_to_eol(p, &headers[num_headers].value, &headers[num_headers].value_len);
        if (ret != RT_PARSE_OK) return ret;
    }

    *header_count = num_headers;

    return RT_PARSE_OK;
}

static void init_req_data(
    rt_req_data *data
) {
    data->method = NULL;
    data->method_len = 0;
    data->path = NULL;
    data->path_len = 0;
    data->version = NULL;
    data->version_len = 0;

    data->header_count = 0;
}

// --------------------------------------------------
// PUBLIC
// --------------------------------------------------
void rt_init_http_parser(
    rt_http_req_parser *p,
    char *start,
    const char *end
) {
    p->start = start;
    p->end = end;

    p->curr = start;
    p->nread = 0;
}

rt_parse_result rt_parse_req(
    rt_http_req_parser *p,
    rt_req_data *data
) {
    CHECK_EOF(); // Check for empty request

    // Init request data with default values
    init_req_data(data);

    // Parse method
    rt_parse_result ret = parse_token(p, &data->method, &data->method_len, ' ');
    if (ret != RT_PARSE_OK) return ret;

    do {
        p->curr++;
        CHECK_EOF();
    } while (*p->curr == ' ');

    // Parse path
    ret = parse_path(p, &data->path, &data->path_len);
    if (ret != RT_PARSE_OK) return ret;

    do {
        p->curr++;
        CHECK_EOF();
    } while (*p->curr == ' ');

    if (data->method_len == 0 || data->path_len == 0) {
        return RT_PARSE_ERR;
    }

    // Parse version
    ret = parse_version(p, &data->version, &data->version_len);
    if (ret != RT_PARSE_OK) return ret;

    if (*p->curr == '\015') { // Expect \r\n
        p->curr++;
        EXPECT_CHAR('\012');
    } else if (*p->curr == '\012') { // Expect \n
        p->curr++;
    } else {
        return RT_INVALID_CHAR;
    }

    // Parse headers
    ret = parse_headers(p, data->headers, &data->header_count);
    if (ret != RT_PARSE_OK) return ret;

    return RT_PARSE_OK;
}