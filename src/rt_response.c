#include "rt_response.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define RESP_HEADERS_BUF_LEN 512
#define RESP_TEXT_MAX_BODY_LEN 1024

// --------------------------------------------------
// PRIVATE
// --------------------------------------------------
static int32_t send_file_resp(
    rt_socket client_fd,
    const char *status,
    const char *content_type,
    const char *file_path
) {
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        return -1;
    }

    struct stat st;
    if (fstat(file_fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(file_fd);
        return -1;
    }

    size_t file_size = (size_t)st.st_size;

    char header[RESP_HEADERS_BUF_LEN];
    int32_t header_len = snprintf(
        header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status,
        content_type,
        file_size
    );

    if (header_len < 0 || header_len >= (int32_t)sizeof(header)) {
        return -1;
    }

    size_t sent = 0;
    size_t header_len_st = (size_t)header_len; // SAFE because header_len >= 0

    while (sent < header_len_st) {
        ssize_t n = send(client_fd, header + sent, header_len_st - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }

    off_t offset = 0;
    while ((size_t)offset < file_size) {
        size_t remaining = file_size - (size_t)offset;
        ssize_t n = sendfile(client_fd, file_fd, &offset, remaining);
        if (n <= 0) return -1;
    }

    return 0;
}

static int32_t send_string_resp(
    rt_socket client_fd,
    const char *status,
    const char *content_type,
    const char *body
) {
    size_t body_len = strlen(body);

    char header[RESP_HEADERS_BUF_LEN];

    int32_t header_len = snprintf(
        header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status,
        content_type,
        body_len
    );

    if (header_len < 0 || header_len >= (int32_t)sizeof(header)) {
        return -1;
    }

    size_t sent = 0;
    size_t header_len_st = (size_t)header_len; // SAFE because header_len >= 0

    while (sent < (size_t)header_len_st) {
        ssize_t n = send(client_fd, header + sent, header_len_st - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }

    sent = 0;
    while (sent < body_len) {
        ssize_t n = send(client_fd, body + sent, body_len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }

    return 0;
}

static int32_t serve_err_reponse(
    rt_socket client_fd,
    const char *status,
    const char *msg
) {
    char body[RESP_TEXT_MAX_BODY_LEN];

    int32_t body_len = snprintf(
        body, sizeof(body),
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
            "<meta charset=\"utf-8\">"
            "<title>%s</title>"
        "</head>"
        "<body>"
            "<div>"
                "<h1>%s</h1>"
                "<p>%s</p>"
                "<p><a href=\"/\">← Back to home</a></p>"
            "</div>"
        "</body>"
        "</html>",
        status,
        status,
        msg
    );

    if (body_len < 0 || body_len >= (int32_t)sizeof(body)) {
        return -1;
    }

    return send_string_resp(
        client_fd,
        status,
        "text/html",
        body
    );
}

// --------------------------------------------------
// PUBLIC
// --------------------------------------------------

void serve_static_file(
    rt_socket client_fd,
    const rt_req_data *req
) {
    (void)req;

    if (req->path_len == 1 && req->path[0] == '/') {
        send_file_resp(
            client_fd,
            RT_STATUS_OK,
            "text/html",
            "./static/index.html"
        );
    }

    // No route found
    serve_err_reponse(
        client_fd,
        RT_STATUS_NOT_FOUND,
        "The requested path doesn't exist"
    );
}