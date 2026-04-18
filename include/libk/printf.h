/**
 * @file printf.h
 * @brief Kernel‑level formatted output routines.
 *
 * This header declares the kernel's minimal printf‑style formatting
 * functions. These implementations do not rely on the standard C
 * library and are safe for freestanding environments. Output is
 * typically routed to VGA, serial, or both depending on backend
 * configuration.
 */

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Formatted Output                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @brief Print a formatted string to the kernel console.
 *
 * Supports a subset of standard printf() format specifiers. Output
 * is directed to the kernel's active console backend(s).
 *
 * @param fmt Format string.
 * @param ... Additional arguments.
 */
void printf(const char *fmt, ...);

/**
 * @brief Print a formatted string using a va_list.
 *
 * Backend used by printf() and panic(). Useful for forwarding
 * variable arguments.
 *
 * @param fmt Format string.
 * @param ap  Argument list.
 */
void vprintf(const char *fmt, va_list ap);

/**
 * @brief Format a string into a buffer with size limitation.
 *
 * Writes at most @p size bytes (including the terminating NUL).
 * Behavior matches standard vsnprintf().
 *
 * @param buf  Destination buffer.
 * @param size Size of the buffer.
 * @param fmt  Format string.
 * @param ap   Argument list.
 * @return Number of characters that would have been written
 *         (excluding NUL), even if truncated.
 */
int vsnprintf(char *buf, usize size, const char *fmt, va_list ap);

/* -------------------------------------------------------------------------- */
/* Diagnostics                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Print a fatal error message and halt the system.
 *
 * Typically used for unrecoverable kernel errors. May disable
 * interrupts and stop execution depending on implementation.
 *
 * @param fmt Format string.
 * @param ... Additional arguments.
 */
void panic(const char *fmt, ...);

/**
 * @brief Dump raw memory in hexadecimal format.
 *
 * Useful for debugging memory corruption, paging issues, or
 * inspecting binary data.
 *
 * @param data Pointer to memory region.
 * @param size Number of bytes to dump.
 */
void hexdump(const void *data, usize size);

/**
 * @brief Internal assertion failure handler.
 *
 * Called by the kernel's assert() macro when an expression evaluates
 * to false. Typically prints diagnostic information and halts.
 *
 * @param expr Expression string.
 * @param file Source file name.
 * @param line Line number.
 */
void assert_fail(const char *expr, const char *file, int line);
