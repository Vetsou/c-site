#ifndef _RT_SOCKET_H_
#define _RT_SOCKET_H_

#include <stdint.h>
#include <netinet/in.h>

typedef int rt_socket;
typedef struct sockaddr_in rt_addr_in;

rt_socket rt_socketTCP(void);
int32_t rt_bind_socket(rt_socket fd, uint16_t port);
int32_t rt_close_socket(rt_socket fd);

#endif // _RT_SOCKET_H_