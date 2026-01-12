#ifndef _RT_RESPONSE_H_
#define _RT_RESPONSE_H_

#include <stddef.h>
#include <stdint.h>

#include "rt_socket.h"

#define RT_STATUS_OK           "200 OK"
#define RT_STATUS_NOT_FOUND    "404 Not Found"
#define RT_STATUS_BAD_REQUEST  "400 Bad Request"
#define RT_STATUS_INTERNAL_ERR "500 Internal Server Error"

int32_t rt_send_file_resp(
    rt_socket client_fd,
    const char *status,
    const char *content_type,
    const char *file_path
);

#endif // _RT_RESPONSE_H_