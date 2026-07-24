/**
 * @file stdarg.h
 * @brief Minimal freestanding implementation of C variable argument handling.
 *
 * This header provides a lightweight replacement for the standard C
 * <stdarg.h> interface, using compiler built‑ins. It is suitable for
 * freestanding environments such as kernels, where the full C library
 * is unavailable.
 *
 * The macros defined here allow functions to accept a variable number
 * of arguments using the familiar `va_list`, `va_start`, `va_arg`,
 * `va_end`, and `va_copy` interfaces.
 */

#pragma once

/**
 * @typedef va_list
 * @brief Opaque type used to iterate over variable arguments.
 *
 * Backed by the compiler's built‑in implementation.
 */
typedef __builtin_va_list va_list;

/**
 * @brief Initialize a `va_list` for argument retrieval.
 *
 * Must be called before using `va_arg()`.
 *
 * @param v The `va_list` to initialize.
 * @param l The last named parameter before the ellipsis.
 */
#define va_start(v,l) __builtin_va_start(v,l)

/**
 * @brief Retrieve the next argument from a `va_list`.
 *
 * @param v The `va_list` being iterated.
 * @param l The type of the next argument.
 * @return The next argument, cast to type `l`.
 */
#define va_arg(v,l)   __builtin_va_arg(v,l)

/**
 * @brief Clean up a `va_list` after use.
 *
 * Must be called before the `va_list` goes out of scope.
 *
 * @param v The `va_list` to finalize.
 */
#define va_end(v)     __builtin_va_end(v)

/**
 * @brief Copy one `va_list` to another.
 *
 * Useful when forwarding variable arguments.
 *
 * @param d Destination `va_list`.
 * @param s Source `va_list`.
 */
#define va_copy(d,s)  __builtin_va_copy(d,s)

