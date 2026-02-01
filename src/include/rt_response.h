#ifndef _RT_RESPONSE_H_
#define _RT_RESPONSE_H_

#include <stddef.h>
#include <stdint.h>

#include "rt_socket.h"
#include "rt_http_parser.h"

#define RT_STATUS_OK           "200 OK"
#define RT_STATUS_BAD_REQUEST  "400 Bad Request"
#define RT_STATUS_FORBIDDEN    "403 Forbidden"
#define RT_STATUS_NOT_FOUND    "404 Not Found"
#define RT_METHOD_NOT_ALLOWED  "405 Method Not Allowed"
#define RT_STATUS_INTERNAL_ERR "500 Internal Server Error"

void handle_request(rt_socket client_fd, const rt_req_data *req, int32_t static_fd);

#endif // _RT_RESPONSE_H_