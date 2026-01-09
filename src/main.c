#include "rt_server.h"

int main(void) {
    rt_server server;
    rt_logger logger;

    rt_init_logger(&logger, "./logs.log");
    rt_init_server(&server, &logger, 8080);
    rt_run_server(&server);

    return 0;
}