#include "rt_server.h"

#include "rt_router.h"
#include "rt_response.h"
#include "rt_http_parser.h"
#include "rt_client_queue.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Config
#define LISTEN_BACKLOG_VAL 32
#define ROUTER_BUCKET_SIZE 128

static void home_handler(
    rt_server *server,
    rt_socket client_fd,
    const rt_req_data *req
) {
    rt_send_file_resp(
        client_fd,
        RT_STATUS_OK,
        "text/html",
        "./static/index.html"
    );
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
    rt_router *router;
} worker_ctx;

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

        // Find route
        rt_route_handler handler = rt_router_find(
            ctx->router,
            req_buffer.path,
            (size_t)req_buffer.path_len
        );

        // Render requested route
        if (handler) {
            handler(ctx->server, client_fd, &req_buffer);
        } else {
            rt_log(ctx->server->logger, LOG_ERROR, "Error sending response");
            close(client_fd);
            free(req_str);
            continue;
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
    rt_logger *logger,
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

    // Create router
    rt_router router;
    if (rt_init_router(&router, ROUTER_BUCKET_SIZE) != 0) {
        rt_log(server->logger, LOG_ERROR, "Error initializing router");
        exit(EXIT_FAILURE);
    }

    rt_router_add(&router, "/home", home_handler);

    // Create server queue context
    rt_client_queue queue;
    rt_init_client_queue(&queue);
    worker_ctx ctx = {
        .server = server,
        .queue = &queue,
        .router = &router
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