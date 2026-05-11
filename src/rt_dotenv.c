#include "rt_dotenv.h"

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char* trim_string(
    char *str
) {
    if (str == NULL) return str;

    while (isspace((unsigned char) *str)) {
        str++;
    }

    if (*str == '\0') return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        end--;
    }

    end[1] = '\0';

    return str;
}

static int is_comment_or_empty(
    const char *line
) {
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return *line == '#' || *line == '\0';
}

int32_t rt_load_dotenv(
    const char *path,
    int32_t overwrite
) {
    if (path == NULL) {
        errno = EINVAL;
        return -1;
    }

    FILE *fd = fopen(path, "rb");
    if (fd == NULL) {
        return -1;
    }

    char *line_buf = NULL;
    size_t len = 0;

    while (getline(&line_buf, &len, fd) != -1) {
        if (is_comment_or_empty(line_buf)) {
            continue;
        }

        // Find '=' symbol
        char *separator = strchr(line_buf, '=');
        if (!separator) continue;

        *separator = '\0';
        char *key = trim_string(line_buf);
        char *value = trim_string(separator + 1);

        if (key == NULL || value == NULL) {
            continue;
        }

        if (setenv(key, value, overwrite) != 0) {
            fclose(fd);
            free(line_buf);
            return -1;
        }
    }

    free(line_buf);
    fclose(fd);
    return 0;
}

const char* rt_getenv_or_default(
    const char *name,
    const char *default_val
) {
    const char *value = getenv(name);
    return (value != NULL) ? value : default_val;
}