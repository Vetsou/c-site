#ifndef _RT_SERVER_H_
#define _RT_SERVER_H_

#include "rt_socket.h"
#include "rt_logger.h"

#include <stdint.h>

/**
 * Set maximum HTTP request size for safety reasons.
 * Default is 4KB.
 */
#ifndef HTTP_MAX_REQ_SIZE
#define HTTP_MAX_REQ_SIZE (8192)
#endif // HTTP_MAX_REQ_SIZE

typedef struct {
    rt_socket socket;
    rt_logger *logger;
    int32_t port;
} rt_server;

void rt_init_server(rt_server *server, rt_logger *logger, uint16_t port);
void rt_run_server(rt_server *server);

#endif // _RT_SERVER_H_