#ifndef _RT_ROUTER_H_
#define _RT_ROUTER_H_

#include "rt_server.h"
#include "rt_http_parser.h"

typedef void (*rt_route_handler)(
    rt_server *server,
    rt_socket client_fd,
    const rt_req_data *req
);

typedef struct rt_route_entry {
    char *path;
    size_t path_len;
    rt_route_handler handler;
    struct rt_route_entry *next;
} rt_route_entry;

typedef struct {
    rt_route_entry **buckets;
    size_t bucket_count;
} rt_router;

int32_t rt_init_router(rt_router *r, size_t bucket_count);
int32_t rt_router_add(rt_router *r, const char *path, rt_route_handler handler);
rt_route_handler rt_router_find(rt_router *r, const char *path, size_t path_len);

#endif // _RT_ROUTER_H_