#include "rt_server.h"

#include "rt_dotenv.h"
#include "rt_response.h"
#include "rt_client_queue.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Config
#define LISTEN_BACKLOG_VAL 32

// --------------------------------------------------
// PRIVATE (Server)
// --------------------------------------------------

static void close_server(
    rt_server *server
) {
    close(server->socket);
    rt_free_logger(server->logger);
}

static int32_t server_accept_conn(
    rt_server *server,
    rt_socket *client_socket
) {
    rt_socket client_fd = accept(server->socket, NULL, NULL);
    if (client_fd == INVALID_SOCKET) {
        return -1;
    }

    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        close(client_fd);
        return -1;
    }

    if (setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
        close(client_fd);
        return -1;
    }

    *client_socket = client_fd;
    return 0;
}

// --------------------------------------------------
// PRIVATE (Send)
// --------------------------------------------------

static char* recv_request(
    rt_server *server,
    rt_socket client_fd,
    rt_req_data *req_buffer
) {
    // Create request buffer
    char *buffer = malloc(HTTP_MAX_REQ_SIZE + 1);
    if (buffer == NULL) {
        rt_log(server->logger, LOG_ERROR, "Error allocating memory for request");
        return NULL;
    }

    // Read request
    ssize_t bytes_read = recv(client_fd, buffer, HTTP_MAX_REQ_SIZE, 0);
    if (bytes_read <= 0) {
        rt_log(server->logger, LOG_ERROR, "Error reading request");
        free(buffer);
        return NULL;
    }

    buffer[bytes_read] = '\0';

    // Parse request
    rt_http_req_parser parser;
    rt_init_http_parser(&parser, buffer, buffer + bytes_read);

    if (rt_parse_req(&parser, req_buffer) != RT_PARSE_OK) {
        rt_log(server->logger, LOG_ERROR, "Error parsing request");
        free(buffer);
        return NULL;
    }

    return buffer;
}

// --------------------------------------------------
// PRIVATE (Handle client thread)
// --------------------------------------------------
typedef struct {
    rt_server *server;
    rt_client_queue *queue;
    int32_t static_fd;
} worker_ctx;

static void init_worker_ctx(
    worker_ctx *ctx,
    rt_server *server,
    rt_client_queue *queue
) {
    const char *static_path = rt_getenv_or_default("RT_STATIC_FILES_PATH", "./static");

    int32_t readonly_fd = open(static_path, O_RDONLY | O_DIRECTORY);
    if (readonly_fd < 0) {
        rt_log(server->logger, LOG_ERROR, "Failed to open 'static' directory for worker context");
        exit(EXIT_FAILURE);
    }

    *ctx = (worker_ctx) {
        .server = server,
        .queue = queue,
        .static_fd = readonly_fd
    };
}

static void* worker_thread(
    void *arg
) {
    worker_ctx *ctx = (worker_ctx*)arg;

    while (1) {
        // Get client
        rt_socket client_fd = rt_queue_pop(ctx->queue);

        // Recv HTTP request
        rt_req_data req_buffer;
        char *req_str = recv_request(ctx->server, client_fd, &req_buffer);
        if (!req_str) {
            close(client_fd);
            continue;
        }

        // Handle static file
        handle_request(client_fd, &req_buffer, ctx->static_fd);

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
    rt_logger *logger,
    uint16_t port
) {
    rt_socket fd = rt_socket_tcp();
    if (fd == INVALID_SOCKET) {
        rt_log(logger, LOG_ERROR, "Error creating server socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        rt_log(logger, LOG_WARN, "Could not set SO_REUSEADDR for server socket");
    }

    if (rt_bind_socket(fd, port) != 0) {
        rt_log(logger, LOG_ERROR, "Error binding server socket");
        close(fd);
        exit(EXIT_FAILURE);
    }

    server->socket = fd;
    server->logger = logger;
    server->port = port;
}

void rt_run_server(
    rt_server *server
) {
    // Start lintening for connections
    if (listen(server->socket, LISTEN_BACKLOG_VAL) != 0) {
        rt_log(server->logger, LOG_ERROR, "Error setting up listen socket");
        close_server(server);
        exit(EXIT_FAILURE);
    }

    // Create server queue context
    rt_client_queue queue;
    rt_init_client_queue(&queue);

    worker_ctx ctx;
    init_worker_ctx(&ctx, server, &queue);

    // Init workers
    pthread_t workers[WORKER_COUNT];
    for (int i = 0; i < WORKER_COUNT; i++) {
        int32_t pthread_result = pthread_create(&workers[i], NULL, worker_thread, &ctx);
        if (pthread_result != 0) {
            rt_log(server->logger, LOG_ERROR, "Error creating worker thread. Code: %d.", pthread_result);
        }
    }

    rt_log(server->logger, LOG_INFO, "Server started on port %d...", server->port);

    // Run server
    while (1) {
        rt_socket client_fd;
        if (server_accept_conn(server, &client_fd) < 0) {
            rt_log(server->logger, LOG_ERROR, "Error accepting client");
            continue;
        }

        // Add new client to queue
        rt_queue_push(&queue, client_fd);
    }

    close(ctx.static_fd);
    close_server(server);
}