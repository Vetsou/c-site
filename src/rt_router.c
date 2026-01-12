#include "rt_router.h"
#include "rt_response.h"

#include <stdlib.h>
#include <string.h>

// --------------------------------------------------
// PRIVATE (Custom Routes)
// --------------------------------------------------

static void not_found_handler(
    rt_server *server,
    rt_socket client_fd,
    const rt_req_data *req
) {
    (void)server; (void)req;
    rt_send_file_resp(
        client_fd,
        RT_STATUS_NOT_FOUND,
        "text/html",
        "./static/not_found.html"
    );
}

// --------------------------------------------------
// PRIVATE (Hash)
// --------------------------------------------------

static uint64_t hash_path(
    const char *path,
    size_t len
) {
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)path[i];
        h *= 1099511628211ULL;
    }
    return h;
}

// --------------------------------------------------
// PUBLIC (Add/Find routes)
// --------------------------------------------------

int32_t rt_init_router(
  rt_router *r,
  size_t bucket_count
) {
    r->buckets = calloc(bucket_count, sizeof(rt_route_entry*));
    if (!r->buckets) return -1;

    r->bucket_count = bucket_count;
    r->error_handler = not_found_handler;
    return 0;
}

int32_t rt_router_add(
  rt_router *r,
  const char *path,
  rt_route_handler handler
) {
    size_t len = strlen(path);
    uint64_t h = hash_path(path, len);
    size_t idx = h % r->bucket_count;

    rt_route_entry *entry = malloc(sizeof(*entry));
    if (!entry) return -1;

    char *path_cpy = strdup(path);
    if (!path_cpy) {
        free(entry);
        return -1;
    }

    entry->path = path_cpy;
    entry->path_len = len;
    entry->handler = handler;

    entry->next = r->buckets[idx];
    r->buckets[idx] = entry;
    return 0;
}

rt_route_handler rt_router_find(
  rt_router *r,
  const char *path,
  size_t path_len
) {
    uint64_t h = hash_path(path, path_len);
    size_t idx = h % r->bucket_count;

    for (rt_route_entry *e = r->buckets[idx]; e; e = e->next) {
        if (e->path_len == path_len && memcmp(e->path, path, path_len) == 0) {
            return e->handler;
        }
    }
    return NULL;
}

// --------------------------------------------------
// PUBLIC (Routes)
// --------------------------------------------------

#define ADD_ROUTE_OR_FAIL(router, path, handler)          \
    do {                                                  \
        if (rt_router_add(router, path, handler) != 0) {  \
            return -1;                                    \
        }                                                 \
    } while(0)

static void home_handler(
    rt_server *server,
    rt_socket client_fd,
    const rt_req_data *req
) {
    (void)server; (void)req;
    rt_send_file_resp(
        client_fd,
        RT_STATUS_OK,
        "text/html",
        "./static/index.html"
    );
}

int32_t rt_setup_all_routes(
    rt_router *r
) {
    ADD_ROUTE_OR_FAIL(r, "/", home_handler);
    return 0;
}