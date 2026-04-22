/**
 * @file errno.h
 * @brief Kernel‑level error codes and strerror() helper.
 *
 * This header defines a minimal set of negative error codes used
 * throughout the kernel and system call layer. Errors are represented
 * as negative integers to distinguish them from valid return values.
 */

#pragma once

/**
 * @enum Errno
 * @brief Standardized kernel error codes.
 *
 * All error values are negative to avoid collision with valid
 * non‑negative return values. These codes are used by system calls,
 * VFS operations, memory allocators, and other kernel subsystems.
 */
typedef enum Errno
{
    ERR_SUCCESS = 0,            /**< No error. */
    ERR_INVALID_ARGUMENT = -1,  /**< Invalid argument provided. */
    ERR_PERMISSION_DENIED = -2, /**< Operation not permitted. */
    ERR_NOT_FOUND = -3,         /**< Requested resource not found. */
    ERR_OUT_OF_MEMORY = -4,     /**< Memory allocation failed. */
    ERR_IO_ERROR = -5,          /**< I/O error occurred. */
    ERR_ALREADY_EXISTS = -6,    /**< Resource already exists. */
    ERR_BUSY = -7,              /**< Resource is busy. */
    ERR_TIMED_OUT = -8,         /**< Operation timed out. */
    ERR_INTERRUPTED = -9,       /**< Operation was interrupted. */
    ERR_NOT_SUPPORTED = -10,    /**< Operation not supported. */
    ERR_UNKNOWN = -11,          /**< Unknown or unspecified error. */
    ENOSYS = -12,               /**< Function not implemented. */
    ENOEXEC = -13,              /**< Exec format error. */
};

/**
 * @brief Convert an Errno value to a human‑readable string.
 *
 * Useful for debugging, logging, and user‑facing error messages.
 *
 * @param err Error code from the Errno enum.
 * @return Pointer to a static string describing the error.
 */
//static inline const char *strerror(Errno err);
