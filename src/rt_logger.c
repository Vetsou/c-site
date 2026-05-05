#include "rt_logger.h"

#include <time.h>
#include <errno.h>
#include <stdlib.h>
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

static int32_t get_timestamp(
    char *buf,
    size_t buf_size
) {
    struct timespec ts;
    struct tm tm;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return -1;
    }

    if (!localtime_r(&ts.tv_sec, &tm)) {
        return -1;
    }

    if (strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", &tm) == 0) {
        return -1;
    }

    return 0;
}

void rt_init_logger(
    rt_logger *logger,
    const char *filepath
) {
    FILE *file = fopen(filepath, "a");
    if (file == NULL) {
        perror("rt_init_logger");
        exit(EXIT_FAILURE);
    }

    logger->file = file;
    pthread_mutex_init(&logger->mutex, NULL);
}

void rt_log(
    rt_logger *logger,
    rt_log_level level,
    const char *msg_format,
    ...
) {
    char log_msg[LOG_LEN_LIMIT] = { 0 };

    char ts[32];
    if (get_timestamp(ts, sizeof(ts)) != 0) {
        snprintf(ts, sizeof(ts), "0000-00-00 00:00:00");
    }

    va_list args;
    va_start(args, msg_format);
    vsnprintf(log_msg, LOG_LEN_LIMIT, msg_format, args);
    va_end(args);

    pthread_mutex_lock(&logger->mutex); // Lock
    fprintf(logger->file, "[%s] [%s] %s\n", ts, get_log_label(level), log_msg);
    fflush(logger->file);
    pthread_mutex_unlock(&logger->mutex); // Unlock
}

void rt_free_logger(rt_logger *logger) {
    if (!logger) return;

    pthread_mutex_destroy(&logger->mutex);

    if (logger->file) {
        fclose(logger->file);
        logger->file = NULL;
    }
}