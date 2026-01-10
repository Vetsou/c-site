#ifndef _RT_LOGGER_H_
#define _RT_LOGGER_H_

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

#define LOG_LEN_LIMIT 256

typedef enum {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR
} rt_log_level;

typedef struct {
    FILE *file;
    pthread_mutex_t mutex;
} rt_logger;

void rt_init_logger(rt_logger *logger, const char *log_filepath);
void rt_log(rt_logger *logger, rt_log_level level, const char *msg_format, ...);
void rt_free_logger(rt_logger *logger);

#endif // _RT_LOGGER_H_