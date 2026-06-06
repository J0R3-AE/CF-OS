#ifndef LIBK_LOG_H
#define LIBK_LOG_H

#include "libk/types.h"
#include <stdarg.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_OKAY,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    LOG_LOG
} log_level_t;

void log_set_level(log_level_t lvl);
void log_set_hide_info(u8 hide);
void log_set_hide_all(u8 hide);

void log_write(log_level_t lvl, const char *fmt, ...);
void log_vwrite(log_level_t lvl, const char *fmt, va_list ap);

void log_write_ex(log_level_t lvl, const char *file, int line, const char *fmt, ...);
void log_vwrite_ex(log_level_t lvl, const char *file, int line, const char *fmt, va_list ap);

/* automatic file/line tagged macros */
#define KLOG_DEBUG(fmt, ...) log_write_ex(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define KLOG_INFO(fmt, ...)  log_write_ex(LOG_INFO,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define KLOG_OKAY(fmt, ...)  log_write_ex(LOG_OKAY,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define KLOG_WARN(fmt, ...)  log_write_ex(LOG_WARN,  __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define KLOG_ERROR(fmt, ...) log_write_ex(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define KLOG_FATAL(fmt, ...) log_write_ex(LOG_FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define KLOG_LOG(fmt, ...)   log_write_ex(LOG_LOG,   __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif