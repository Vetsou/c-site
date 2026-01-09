#include "rt_logger.h"

#include <stdio.h>
#include <stdarg.h>

static const char* get_log_label(
    rt_log_level level
) {
    switch (level) {
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARNING";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

void rt_init_logger(
    rt_logger *logger,
    const char *log_filepath
) {
    logger->filepath = log_filepath;
}

void rt_log(
    const rt_logger *logger,
    rt_log_level level,
    const char *msg_format,
    ...
) {
    char log_msg[LOG_LEN_LIMIT] = { 0 };

    va_list args;
    va_start(args, msg_format);
    vsnprintf(log_msg, LOG_LEN_LIMIT, msg_format, args);
    va_end(args);

    FILE *file = fopen(logger->filepath, "a");
    if (file) {
        fprintf(file, "[%s] %s\n", get_log_label(level), log_msg);
        fclose(file);
    }
}