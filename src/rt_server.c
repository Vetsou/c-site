#include "rt_server.h"
#include "rt_http_parser.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h> // TODO: Delete

// --------------------------------------------------
// PRIVATE
// --------------------------------------------------
#define LISTEN_BACKLOG_VAL 32

static char* recv_request(
    rt_server *server,
    rt_socket client_fd,
    ssize_t *read_size
) {
    char *buffer = (char *)malloc(HTTP_MAX_REQ_SIZE + 1);
    if (buffer == NULL) {
        rt_log(server->logger, LOG_ERROR, "Error allocating memory for request");
        return NULL;
    }

    ssize_t bytes_read = recv(client_fd, buffer, HTTP_MAX_REQ_SIZE, 0);

    if (bytes_read <= 0) {
        rt_log(server->logger, LOG_ERROR, "Error reading request");
        free(buffer);
        return NULL;
    }

    *read_size = bytes_read;
    buffer[bytes_read] = '\0';
    return buffer;
}

// --------------------------------------------------
// PUBLIC
// --------------------------------------------------
void rt_init_server(
    rt_server *server,
    const rt_logger *logger,
    uint16_t port
) {
    rt_socket fd = rt_socketTCP();
    if (fd == INVALID_SOCKET) {
        rt_log(logger, LOG_ERROR, "Error creating server socket");
        exit(EXIT_FAILURE);
    }

    if (rt_bind_socket(fd, port) != 0) {
        rt_log(logger, LOG_ERROR, "Error binding server socket");
        exit(EXIT_FAILURE);
    }

    server->logger = logger;
    server->port = port;
    server->socket = fd;
}

void rt_run_server(
    rt_server *server
) {
    if (listen(server->socket, LISTEN_BACKLOG_VAL) != 0) {
        rt_log(server->logger, LOG_ERROR, "Error setting up listen socket");
        exit(EXIT_FAILURE);
    }

    rt_log(server->logger, LOG_INFO, "Server started on port %d...", server->port);

    while (1) {
        // Accept client
        rt_socket client_fd = accept(server->socket, NULL, NULL);
        if (client_fd == INVALID_SOCKET) {
            rt_log(server->logger, LOG_ERROR, "Error accepting client");
            continue;
        }

        // Read request
        ssize_t size_read;
        char *req_str = recv_request(server, client_fd, &size_read);
        if (req_str == NULL) {
            close(client_fd);
            continue;
        }

        // Parse http request
        rt_http_req_parser parser;
        rt_req_data d;

        rt_init_http_parser(&parser, req_str, req_str + size_read);
        rt_parse_result r = rt_parse_req(&parser, &d);
        if (r != RT_PARSE_OK) {
            rt_log(server->logger, LOG_ERROR, "Error parsing request");
            close(client_fd);
            free(req_str);
            continue;
        }

        // TODO: Delete this. Debug logs
        printf("DATA:\n");

        printf("  Method : |%.*s|\n", d.method_len, d.method);
        printf("  Path   : |%.*s|\n", d.path_len, d.path);
        printf("  Version: |%.*s|\n", d.version_len, d.version);

        printf("HEADERS:\n");
        for (int i = 0; i < d.header_count; i++) {
            printf("  Header (%d) |%.*s| |%.*s|\n", i,
                d.headers[i].name_len, d.headers[i].name,
                d.headers[i].value_len, d.headers[i].value);
        }

        // Send response
        const char *resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 21\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<h1>Hello World!</h1>";

        send(client_fd, resp, strlen(resp), 0);

        // Cleanup
        close(client_fd);
        free(req_str);
    }
}