/**
 * @file printf.h
 * @brief Kernel-level formatted output routines.
 */

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Formatted Output                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Print a formatted string to kernel console.
 */
void printf(const char *fmt, ...);

/**
 * @brief Print formatted string using va_list.
 */
void vprintf(const char *fmt, va_list ap);

/**
 * @brief Format string into buffer (bounded).
 */
int snprintf(char *buf, usize size, const char *fmt, ...);

/**
 * @brief Format string into buffer using va_list.
 */
int vsnprintf(char *buf, usize size, const char *fmt, va_list ap);

/* -------------------------------------------------------------------------- */
/* Simple Output Helpers                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Print string + newline.
 */
void puts(const char *s);

/**
 * @brief Print single character.
 */
void putchar(int c);

/* -------------------------------------------------------------------------- */
/* Diagnostics                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Print fatal error and halt system.
 */
void panic(const char *fmt, ...);

/**
 * @brief Hex dump memory region (debugging).
 */
void hexdump(const void *data, usize size);

/**
 * @brief Assertion failure handler.
 */
void assert_fail(const char *expr, const char *file, int line);