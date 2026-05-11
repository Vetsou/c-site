#include "rt_dotenv.h"
#include "rt_server.h"

int main(void) {
    if (rt_load_dotenv(".env", 1) != 0) {
        perror("rt_load_dotenv");
    }

    const char *logs_file = rt_getenv_or_default("RT_LOGS_FILE_PATH", "./logs.log");

    rt_logger logger;
    rt_init_logger(&logger, logs_file);

    rt_server server;
    rt_init_server(&server, &logger, 8080);
    rt_run_server(&server);

    return 0;
}