#include "rt_socket.h"
#include <unistd.h>

rt_socket rt_socket_tcp() {
    rt_socket fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return fd;
}

int32_t rt_bind_socket(
    rt_socket fd,
    uint16_t port
) {
    rt_addr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    return bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
}