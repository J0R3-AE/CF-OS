/**
 * @file types.h
 * @brief Core fixed‑width integer, pointer‑sized, and utility type
 *        definitions used throughout the kernel.
 *
 * This header provides architecture‑independent typedefs for signed and
 * unsigned integers, floating‑point types, pointer‑sized types, and
 * common utility macros. These definitions ensure consistent type sizes
 * across the entire kernel regardless of compiler or platform.
 */

#pragma once
#include <stdbool.h>

/// @section signed integer types

/// @typedef s8
/// @brief 8‑bit signed integer.
typedef signed char         s8;

/// @typedef s16
/// @brief 16‑bit signed integer.
typedef signed short        s16;

/// @typedef s32
/// @brief 32‑bit signed integer.
typedef signed int          s32;

/// @typedef s64
/// @brief 64‑bit signed integer.
typedef signed long long    s64;

/// @section unsigned integer types

/// @typedef u8
/// @brief 8‑bit unsigned integer.
typedef unsigned char       u8;

/// @typedef u16
/// @brief 16‑bit unsigned integer.
typedef unsigned short      u16;

/// @typedef u32
/// @brief 32‑bit unsigned integer.
typedef unsigned int        u32;

/// @typedef u64
/// @brief 64‑bit unsigned integer
typedef unsigned long long  u64;

/// @section floating‑point types

/// @typedef f32
/// @brief 32‑bit single‑precision floating‑point.
typedef float               f32;

/// @typedef f64
/// @brief 64‑bit double‑precision floating‑point.
typedef double              f64;

/* -------------------------------------------------------------------------- */
/* Pointer‑Sized Types                                                        */
/* -------------------------------------------------------------------------- */

/// @typedef usize
/// @brief Unsigned integer capable of holding a pointer difference.
/* Equivalent to `size_t`. */
typedef __SIZE_TYPE__       usize;

/// @typedef ssize
/// @brief Signed integer capable of holding a pointer difference.
/* Equivalent to `ssize_t`. */
typedef __PTRDIFF_TYPE__    ssize;

/// @typedef uptr
/// @brief Unsigned integer capable of holding a pointer value.
/* Equivalent to `uintptr_t`. */
typedef __UINTPTR_TYPE__    uptr;

/// @typedef sptr
/// @brief Signed integer capable of holding a pointer value.
/* Equivalent to `intptr_t`. */
typedef __INTPTR_TYPE__     sptr;

/// @typedef ptrdiff
/// @brief Signed integer type for pointer differences.
/* Equivalent to `ptrdiff_t`. */
typedef __PTRDIFF_TYPE__    ptrdiff;

/// @section boolean type

/// @typedef b8
/// @brief 8‑bit boolean type.
typedef bool                b8;

#define true  1  /**< Boolean true. */
#define false 0  /**< Boolean false. */

/* -------------------------------------------------------------------------- */
/* Attributes                                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Mark a struct as packed (no padding between members).
 */
#define PACKED  __attribute__((__packed__))

/**
 * @brief Force alignment of a variable or struct to `x` bytes.
 */
#define ALIGNED(x) __attribute__((aligned(x)))

/**
 * @brief Compiler extension for type inference (GCC/Clang).
 */
#define typeof  __typeof__

/* -------------------------------------------------------------------------- */
/* Size Macros                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Size constants for readability.
 */
#define KB 1024ULL
#define MB (1024ULL * 1024ULL)
#define GB (1024ULL * 1024ULL * 1024ULL)
#define TB (1024ULL * 1024ULL * 1024ULL * 1024ULL)

