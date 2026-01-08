#include "rt_socket.h"
#include <unistd.h>

/**
 * Creates a TCP socket.
 *
 * Returns:
 *   SUCCESS: Socket file descriptor
 *   FAILURE: -1 / INVALID_SOCKET (errno is set)
 */
rt_socket rt_socketTCP() {
    rt_socket fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return fd;
}

/**
 * Binds a socket to a local port on all available network interfaces.
 *
 * Parameters:
 *   fd   - Socket file descriptor
 *   port - port number
 *
 * Returns:
 *   SUCCESS: 0
 *   FAILURE: -1 (errno is set)
 */
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

/**
 * Closes an open socket.
 *
 * Parameters:
 *   fd - Socket file descriptor to close
 *
 * Returns:
 *   SUCCESS: 0
 *   FAILURE: -1 (errno is set)
 */
int32_t rt_close_socket(
    rt_socket fd
) {
    return close(fd);
}