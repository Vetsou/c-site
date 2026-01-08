#ifndef _RT_LOGGER_H_
#define _RT_LOGGER_H_

#include <stdint.h>

#define LOG_LEN_LIMIT 256

typedef enum {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR
} rt_log_level;

typedef struct {
    const char *filepath;
} rt_logger;

void rt_init_logger(rt_logger *logger, const char *log_filepath);
void rt_log(rt_logger *logger, rt_log_level level, const char *msg_format, ...);

#endif // _RT_LOGGER_H_