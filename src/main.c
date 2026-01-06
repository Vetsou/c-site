#include "rt_socket.h"
#include "rt_http_parser.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    //rt_socket fd = rt_socketTCP();
    //rt_bind_socket(fd, 8080);
    //rt_close_socket(fd);

    char *data = "GET /path HTTP/1.0\r\nContent: json\r\nMY_CUSTOM_HEADER: cool-value\r\nUnderscore-header: _W_O_W_\r\n\r\n";

    rt_http_req_parser p;
    rt_req_data d;

    rt_init_http_parser(&p, data, data + strlen(data));
    rt_parse_result r = rt_parse_req(&p, &d);

    printf("Result: %d\n", r);

    if (r == RT_PARSE_OK) {
        printf("DATA:\n");

        printf("  Method : |%.*s|\n", d.method_len, d.method);
        printf("  Path   : |%.*s|\n", d.path_len, d.path);
        printf("  Version: |%.*s|\n", d.version_len, d.version);

        printf("HEADERS:\n");
        for (int i = 0; i < d.header_count; i++) {
            printf("  Header (%d) |%.*s| |%.*s|\n", i,
                d.headers[i].name_len, d.headers[i].name,
                d.headers[i].value_len, d.headers[i].value);
        }
    }

    return 0;
}