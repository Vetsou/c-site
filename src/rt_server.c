#include "rt_server.h"

#include "rt_http_parser.h"
#include "rt_client_queue.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

typedef struct {
    rt_server *server;
    rt_client_queue *queue;
} worker_ctx;

static void* worker_thread(
    void *arg
) {
    worker_ctx *ctx = (worker_ctx*)arg;

    while (1) {
        // Get client
        rt_socket client_fd = rt_queue_pop(ctx->queue);

        // Recv HTTP request
        ssize_t size_read;
        char *req_str = recv_request(ctx->server, client_fd, &size_read);
        if (!req_str) {
            close(client_fd);
            continue;
        }

        // Parse HTTP request
        rt_req_data d;
        rt_http_req_parser parser;
        rt_init_http_parser(&parser, req_str, req_str + size_read);

        if (rt_parse_req(&parser, &d) != RT_PARSE_OK) {
            //rt_log(ctx->server->logger, LOG_ERROR, "Error parsing request");
            close(client_fd);
            free(req_str);
            continue;
        }

        // Send HTTP response
        // TODO: Refactor to return files
        const char *resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: 24\r\n"
            "Connection: close\r\n"
            "\r\n"
            "<h1>Hello World !!!</h1>";

        size_t total_send = 0;
        size_t resp_len = strlen(resp);

        while (total_send < resp_len) {
            ssize_t n = send(client_fd, resp + total_send, resp_len - total_send, 0);
            if (n <= 0) {
                rt_log(ctx->server->logger, LOG_ERROR, "Error sending response");
                break;
            }
            total_send += (size_t)n;
        }

        close(client_fd);
        free(req_str);
    }
    return NULL;
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
    // Start lintening for connections
    if (listen(server->socket, LISTEN_BACKLOG_VAL) != 0) {
        rt_log(server->logger, LOG_ERROR, "Error setting up listen socket");
        exit(EXIT_FAILURE);
    }

    // Create server queue context
    rt_client_queue queue;
    rt_init_client_queue(&queue);
    worker_ctx ctx = {
        .server = server,
        .queue = &queue
    };

    // Init workers
    pthread_t workers[WORKER_COUNT];
    for (int i = 0; i < WORKER_COUNT; i++) {
        pthread_create(&workers[i], NULL, worker_thread, &ctx);
    }

    rt_log(server->logger, LOG_INFO, "Server started on port %d...", server->port);

    // Run server
    while (1) {
        rt_socket client_fd = accept(server->socket, NULL, NULL);
        if (client_fd == INVALID_SOCKET) {
            rt_log(server->logger, LOG_ERROR, "Error accepting client");
            continue;
        }

        // Add new client to queue
        rt_queue_push(&queue, client_fd);
    }
}