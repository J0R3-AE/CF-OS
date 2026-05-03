/**
 * @file kernel_log.h
 * @brief Kernel logging system with multiple log levels and dual output
 *        (serial + VGA).
 *
 * This module provides a lightweight logging facility for the kernel.
 * Messages are filtered by log level and can be emitted to multiple backends.
 */

#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <stdarg.h>
#include <stdbool.h>

/**
 * @enum log_level_t
 * @brief Logging severity levels used by the kernel logger.
 *
 * The logger will only emit messages at or above the currently configured
 * log level. For example, if the level is set to LOG_WARN, then LOG_DEBUG
 * and LOG_INFO messages will be suppressed.
 */
typedef enum {
    LOG_DEBUG,  /**< Verbose debugging information. */
    LOG_INFO,   /**< General informational messages. */
    LOG_WARN,   /**< Warnings about potential issues. */
    LOG_ERROR,  /**< Errors that do not halt the system. */
    LOG_FATAL,  /**< Critical errors; may halt the system. */
    LOG_LOG     /**< Miscellaneous messages not tied to severity. */
} log_level_t;

/**
 * @brief Set the global log level.
 *
 * Messages below this level will be ignored.
 *
 * @param lvl The minimum log level to output.
 */
void log_set_level(log_level_t lvl);

/**
 * @brief Log a formatted message at the specified log level.
 *
 * This is the primary logging function used by the convenience macros.
 * It behaves similarly to printf(), but outputs to kernel logging backends.
 *
 * @param lvl The severity level of the message.
 * @param fmt printf-style format string.
 * @param ... Additional arguments for formatting.
 */
void log_write(log_level_t lvl, const char *fmt, ...);

/**
 * @brief Log a formatted message using a va_list.
 *
 * This is the internal backend used by log_write() and is useful when
 * forwarding variadic arguments.
 *
 * @param lvl The severity level of the message.
 * @param fmt printf-style format string.
 * @param ap  va_list containing the arguments.
 */
void log_vwrite(log_level_t lvl, const char *fmt, va_list ap);

/* -------------------------------------------------------------------------- */
/* Convenience macros                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Log a debug-level message.
 */
#define klog_debug(fmt, ...) log_write(LOG_DEBUG, fmt, ##__VA_ARGS__)

/**
 * @brief Log an informational message.
 */
#define klog_info(fmt, ...)  log_write(LOG_INFO,  fmt, ##__VA_ARGS__)

/**
 * @brief Log a warning message.
 */
#define klog_warn(fmt, ...)  log_write(LOG_WARN,  fmt, ##__VA_ARGS__)

/**
 * @brief Log an error message.
 */
#define klog_err(fmt, ...)   log_write(LOG_ERROR, fmt, ##__VA_ARGS__)

/**
 * @brief Log a fatal error message.
 *
 * Fatal messages are always shown and may halt the system depending on
 * implementation.
 */
#define klog_fatal(fmt, ...) log_write(LOG_FATAL, fmt, ##__VA_ARGS__)

/**
 * @brief Log a miscellaneous message.
 */
#define klog_log(fmt, ...)  log_write(LOG_LOG, fmt, ##__VA_ARGS__)

#endif /* KERNEL_LOG_H */
