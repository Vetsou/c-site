#include "rt_response.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sendfile.h>

#include <linux/openat2.h>

// --------------------------------------------------
// DEFINES
// --------------------------------------------------

// Maximum size of file path in request
#define REQ_MAX_PATH_LEN 512

// Response headers buffer size
#define RESP_HEADERS_BUF_LEN 512

// Max size of body for text responses
#define RESP_TEXT_MAX_BODY_LEN 1024

// Max size of body for file responses
#define RESP_MAX_FILE_SIZE (5 * 1024 * 1024)

// --------------------------------------------------
// PRIVATE (Utils)
// --------------------------------------------------

typedef enum {
    RT_UNKNOWN_METHOD = -1,
    RT_GET_METHOD = 0
} req_method_t;

static req_method_t get_request_method(
    const rt_req_data *req
) {
    if (req->method_len == 3 && strncmp(req->method, "GET", 3) == 0) {
        return RT_GET_METHOD;
    }

    return RT_UNKNOWN_METHOD;
}

static const char* mime_from_path(
    const char *path
) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";

    if (!strncmp(ext, ".html", 4)) return "text/html";
    if (!strncmp(ext, ".css", 3))  return "text/css";
    if (!strncmp(ext, ".png", 3))  return "image/png";
    if (!strncmp(ext, ".jpg", 3))  return "image/jpeg";
    if (!strncmp(ext, ".svg", 3))  return "image/svg+xml";
    if (!strncmp(ext, ".gif", 3))  return "image/gif";

    return "application/octet-stream";
}

static int32_t openat2_safe(
    int32_t dir_fd,
    const char *path,
    uint64_t flags
) {
    struct open_how how = {
        .flags = flags,
        .resolve =
            RESOLVE_BENEATH |
            RESOLVE_NO_SYMLINKS |
            RESOLVE_NO_MAGICLINKS |
            RESOLVE_NO_XDEV
    };

    long ret = syscall(SYS_openat2, dir_fd, path, &how, sizeof(how));

    if (ret < 0) return -1;
    return (int32_t)ret;
}

// --------------------------------------------------
// PRIVATE (Returning files)
// --------------------------------------------------

static int32_t send_file_resp(
    rt_socket client_fd,
    const char *status,
    const char *content_type,
    int file_fd,
    off_t file_size
) {
    char header[RESP_HEADERS_BUF_LEN];

    if (file_size < 0 || file_size > RESP_MAX_FILE_SIZE) return -1;

    int32_t header_len = snprintf(
        header, sizeof(header),
        "HTTP/1.1 %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        status,
        content_type,
        (size_t)file_size
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
    while (offset < file_size) {
        size_t remaining = (size_t)(file_size - offset);
        ssize_t n = sendfile(client_fd, file_fd, &offset, remaining);
        if (n <= 0) return -1;
    }

    return 0;
}

static int32_t send_string_resp(
    rt_socket client_fd,
    const char *status,
    const char *content_type,
    const char *body,
    size_t body_len
) {
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

    while (sent < header_len_st) {
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
                "<p><a href=\"/\">Back to home</a></p>"
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
        body,
        (size_t)body_len
    );
}

// --------------------------------------------------
// PRIVATE (Handling methods)
// --------------------------------------------------

static void handle_get(
    rt_socket client_fd,
    const rt_req_data *req,
    int32_t static_fd
) {
    char rel_path[REQ_MAX_PATH_LEN];

    // Check path size
    if (!req->path || req->path_len <= 0 || req->path_len >= REQ_MAX_PATH_LEN) {
        serve_err_reponse(client_fd, RT_STATUS_BAD_REQUEST, "Invalid path");
        return;
    }

    // Copy path
    memcpy(rel_path, req->path, (size_t)req->path_len);
    rel_path[req->path_len] = '\0';

    // Check root dir and first char '/'
    const char *file = strcmp(rel_path, "/") == 0
        ? "index.html"
        : (rel_path[0] == '/' ? rel_path + 1 : rel_path);

    int32_t file_fd = openat2_safe(static_fd, file, O_RDONLY | O_CLOEXEC);
    if (file_fd < 0) {
        serve_err_reponse(client_fd, RT_STATUS_NOT_FOUND, "File not found");
        return;
    }

    // Check for symlink
    struct stat st;
    if (fstat(file_fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(file_fd);
        serve_err_reponse(client_fd, RT_STATUS_FORBIDDEN, "Access denied");
        return;
    }

    const char *mime = mime_from_path(file);
    if (send_file_resp(client_fd, RT_STATUS_OK, mime, file_fd, st.st_size) < 0) {
        close(file_fd);
        serve_err_reponse(client_fd, RT_STATUS_INTERNAL_ERR, "Failed to return file");
        return;
    }

    close(file_fd);
}

// --------------------------------------------------
// PUBLIC
// --------------------------------------------------

void handle_request(
    rt_socket client_fd,
    const rt_req_data *req,
    int32_t static_fd
) {
    switch (get_request_method(req)) {
        case RT_GET_METHOD:
            handle_get(client_fd, req, static_fd);
            break;

        case RT_UNKNOWN_METHOD:
        default:
            serve_err_reponse(client_fd, RT_METHOD_NOT_ALLOWED, "Unknown or not allowed method");
            break;
    }
}