#include "rt_response.h"

#include <stdio.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define RESP_HEADERS_BUF_LEN 512

int32_t rt_send_file_resp(
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