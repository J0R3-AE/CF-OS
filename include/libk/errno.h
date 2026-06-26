/**
 * @file errno.h
 * @brief Kernel-level error codes and strerror() helper.
 *
 * Defines a structured set of negative error codes used across the kernel:
 * system calls, VFS, memory management, device drivers, and process control.
 */

#pragma once

/// @brief Kernel-level error codes used across the kernel. Negative values indicate errors, while zero indicates success. These codes are used for system calls, VFS, memory management, device drivers, and process control.
typedef enum Errno
{
    /* Success */
    ERR_SUCCESS = 0,              /* Operation completed successfully */

    /* Generic */
    ERR_UNKNOWN             = -1, /* Unknown error */
    ERR_INVALID_ARGUMENT    = -2, /* Invalid argument */
    ERR_NOT_SUPPORTED       = -3, /* Operation not supported */
    ERR_PERMISSION_DENIED   = -4, /* Permission denied */
    ERR_INTERRUPTED         = -5, /* Operation interrupted */
    ERR_TIMED_OUT           = -6, /* Operation timed out */
    ERR_BUSY                = -7, /* Resource busy */

    /* Memory Management */
    ERR_OUT_OF_MEMORY       = -10, /* No free memory available */
    ERR_BAD_ADDRESS         = -11, /* Invalid virtual/physical address */
    ERR_BUFFER_TOO_SMALL    = -12, /* Buffer too small for operation */

    ERR_PAGE_FAULT          = -13, /* Page not present or protection fault */
    ERR_PAGE_ALREADY_MAPPED = -14, /* Attempted to map an already mapped page */
    ERR_PAGE_NOT_MAPPED     = -15, /* Attempted to unmap/query unmapped page */

    ERR_FRAME_UNAVAILABLE   = -16, /* PMM has no free frames */
    ERR_INVALID_ALIGNMENT   = -17, /* Alignment requirement not met */
    ERR_HEAP_CORRUPTED      = -18, /* Heap metadata damaged */
    ERR_HEAP_EXHAUSTED      = -19, /* Heap reached maximum size */

    ERR_PROTECTION_FAULT    = -20, /* Access violates page permissions */
    ERR_ADDRESS_IN_USE      = -21, /* Virtual region already occupied */
    ERR_ADDRESS_NOT_MAPPED  = -22, /* Region not mapped */

    ERR_STACK_OVERFLOW      = -23, /* Stack grew beyond limit */
    ERR_STACK_UNDERFLOW     = -24, /* Stack underflow detected */

    ERR_SEGMENTATION_FAULT  = -25, /* Invalid memory access */
    ERR_DOUBLE_FREE         = -26, /* Free called twice on same block */
    ERR_INVALID_FREE        = -27, /* Pointer not allocated by allocator */

    /* Filesystem */
    ERR_NOT_FOUND           = -40, /* File or directory not found */
    ERR_ALREADY_EXISTS      = -41, /* File or directory already exists */
    ERR_NOT_DIRECTORY       = -42, /* Path is not a directory */
    ERR_IS_DIRECTORY        = -43, /* Path is a directory */
    ERR_DIRECTORY_NOT_EMPTY = -44, /* Directory is not empty */
    ERR_READ_ONLY_FS        = -45, /* Filesystem is read-only */
    ERR_NO_SPACE_LEFT       = -46, /* No space left on device */
    ERR_NAME_TOO_LONG       = -47, /* Name too long */

    /* File descriptors */
    ERR_BAD_FILE_DESCRIPTOR = -60, /* Bad file descriptor */
    ERR_TOO_MANY_OPEN_FILES = -61, /* Too many open files */
    ERR_END_OF_FILE         = -62, /* End of file */

    /* I/O */
    ERR_IO_ERROR            = -80, /* I/O error */
    ERR_DEVICE_NOT_FOUND    = -81, /* Device not found */
    ERR_DEVICE_BUSY         = -82, /* Device busy */
    ERR_DEVICE_FAULT        = -83, /* Device fault */

    /* Processes */
    ERR_NO_CHILD_PROCESS    = -100, /* No child process */
    ERR_PROCESS_NOT_FOUND   = -101, /* Process not found */
    ERR_EXEC_FORMAT_ERROR   = -102, /* Execution format error */

    /* Networking */
    ERR_NOT_CONNECTED       = -120, /* Not connected */
    ERR_CONNECTION_REFUSED  = -121, /* Connection refused */
    ERR_CONNECTION_RESET    = -122, /* Connection reset */
    ERR_ADDRESS_IN_USE_NET  = -123, /* separate from memory-region address */

    /* Kernel */
    ERR_PANIC               = -200, /* Panic */
    ERR_INTERNAL            = -201 /* Internal error */

} Errno_t;

/// @section Defines of enums (POSIX style)
#define EINVAL       ERR_INVALID_ARGUMENT
#define ENOTSUP      ERR_NOT_SUPPORTED
#define EPERM        ERR_PERMISSION_DENIED
#define EINTR        ERR_INTERRUPTED
#define ETIMEDOUT    ERR_TIMED_OUT
#define EBUSY        ERR_BUSY

/* Memory */
#define ENOMEM       ERR_OUT_OF_MEMORY
#define EFAULT       ERR_BAD_ADDRESS
#define ERANGE       ERR_BUFFER_TOO_SMALL

/* Filesystem */
#define ENOENT       ERR_NOT_FOUND
#define EEXIST       ERR_ALREADY_EXISTS
#define ENOTDIR      ERR_NOT_DIRECTORY
#define EISDIR       ERR_IS_DIRECTORY
#define ENOTEMPTY    ERR_DIRECTORY_NOT_EMPTY
#define EROFS        ERR_READ_ONLY_FS
#define ENOSPC       ERR_NO_SPACE_LEFT
#define ENAMETOOLONG ERR_NAME_TOO_LONG

/* File descriptors */
#define EBADF        ERR_BAD_FILE_DESCRIPTOR
#define EMFILE       ERR_TOO_MANY_OPEN_FILES

/* I/O */
#define EIO          ERR_IO_ERROR
#define ENODEV       ERR_DEVICE_NOT_FOUND

/* Processes */
#define ECHILD       ERR_NO_CHILD_PROCESS
#define ESRCH        ERR_PROCESS_NOT_FOUND
#define ENOEXEC      ERR_EXEC_FORMAT_ERROR

/* Networking */
#define ENOTCONN     ERR_NOT_CONNECTED
#define ECONNREFUSED ERR_CONNECTION_REFUSED
#define ECONNRESET   ERR_CONNECTION_RESET
#define EADDRINUSE   ERR_ADDRESS_IN_USE_NET


/// @brief Convert an Errno value to a human-readable string.
/// @param err The error code to convert.
/// @return A pointer to the error string.
#define IS_ERR(x) ((x) < 0)
#define PTR_ERR(x) ((int)(intptr_t)(x))
#define ERR_PTR(x) ((void *)(intptr_t)(x))

/// @brief Universal success-or-error propagation
/// @param x The error code to check.
/// @return The error code if it indicates an error, otherwise a success value.
#define ERR_SUCCESS_PTR  ((void *)0)
#define ERR_SUCCESS_INT  0
#define ERR_SUCCESS_VOID ((void)0)
#define ERR_SUCCESS_BOOL true

/// @brief Universal success-or-error propagation
/// @param x The error code to check.
/// @return The error code if it indicates an error, otherwise a success value.
#define ERR_SUCCESS_OR(x)        ((x) < 0 ? (x) : ERR_SUCCESS_INT)
#define ERR_SUCCESS_OR_PTR(x)    ((x) < 0 ? (void *)(intptr_t)(x) : ERR_SUCCESS_PTR)
#define ERR_SUCCESS_OR_BOOL(x)   ((x) < 0 ? (x) : ERR_SUCCESS_BOOL)
#define ERR_SUCCESS_OR_VOID(x)   ((x) < 0 ? (x) : ERR_SUCCESS_VOID)
#define ERR_SUCCESS_OR_ERRNO(x)  ((x) < 0 ? (x) : ERR_SUCCESS_INT)

/// @brief Universal success-or-error propagation with fallback value
/// @param x The error code to check.
/// @param y The fallback value to use if the error code indicates success.
/// @return The error code if it indicates an error, otherwise the fallback value.
#define ERR_SUCCESS_OR_ERRNO_OR(x, y) ((x) < 0 ? (x) : (y))

/// @brief Convert an Errno value to a human-readable string.
/// @param err The error code to convert.
/// @return A pointer to the error string.
static inline const char *strerror(int err)
{
    switch (err)
    {
        case ERR_SUCCESS: return "Success";

        /* Generic */
        case ERR_UNKNOWN: return "Unknown error";
        case ERR_INVALID_ARGUMENT: return "Invalid argument";
        case ERR_NOT_SUPPORTED: return "Operation not supported";
        case ERR_PERMISSION_DENIED: return "Permission denied";
        case ERR_INTERRUPTED: return "Operation interrupted";
        case ERR_TIMED_OUT: return "Operation timed out";
        case ERR_BUSY: return "Resource busy";

        /* Memory Management */
        case ERR_OUT_OF_MEMORY: return "Out of memory";
        case ERR_BAD_ADDRESS: return "Bad address";
        case ERR_BUFFER_TOO_SMALL: return "Buffer too small";

        case ERR_PAGE_FAULT: return "Page fault";
        case ERR_PAGE_ALREADY_MAPPED: return "Page already mapped";
        case ERR_PAGE_NOT_MAPPED: return "Page not mapped";

        case ERR_FRAME_UNAVAILABLE: return "No free physical frames";
        case ERR_INVALID_ALIGNMENT: return "Invalid alignment";
        case ERR_HEAP_CORRUPTED: return "Heap corrupted";
        case ERR_HEAP_EXHAUSTED: return "Heap exhausted";

        case ERR_PROTECTION_FAULT: return "Protection fault";
        case ERR_ADDRESS_IN_USE: return "Address already in use";
        case ERR_ADDRESS_NOT_MAPPED: return "Address not mapped";

        case ERR_STACK_OVERFLOW: return "Stack overflow";
        case ERR_STACK_UNDERFLOW: return "Stack underflow";

        case ERR_SEGMENTATION_FAULT: return "Segmentation fault";
        case ERR_DOUBLE_FREE: return "Double free detected";
        case ERR_INVALID_FREE: return "Invalid free";

        /* Filesystem */
        case ERR_NOT_FOUND: return "Not found";
        case ERR_ALREADY_EXISTS: return "Already exists";
        case ERR_NOT_DIRECTORY: return "Not a directory";
        case ERR_IS_DIRECTORY: return "Is a directory";
        case ERR_DIRECTORY_NOT_EMPTY: return "Directory not empty";
        case ERR_READ_ONLY_FS: return "Read-only filesystem";
        case ERR_NO_SPACE_LEFT: return "No space left";
        case ERR_NAME_TOO_LONG: return "Name too long";

        /* File descriptors */
        case ERR_BAD_FILE_DESCRIPTOR: return "Bad file descriptor";
        case ERR_TOO_MANY_OPEN_FILES: return "Too many open files";
        case ERR_END_OF_FILE: return "End of file";

        /* I/O */
        case ERR_IO_ERROR: return "I/O error";
        case ERR_DEVICE_NOT_FOUND: return "Device not found";
        case ERR_DEVICE_BUSY: return "Device busy";
        case ERR_DEVICE_FAULT: return "Device fault";

        /* Processes */
        case ERR_NO_CHILD_PROCESS: return "No child process";
        case ERR_PROCESS_NOT_FOUND: return "Process not found";
        case ERR_EXEC_FORMAT_ERROR: return "Exec format error";

        /* Networking */
        case ERR_NOT_CONNECTED: return "Not connected";
        case ERR_CONNECTION_REFUSED: return "Connection refused";
        case ERR_CONNECTION_RESET: return "Connection reset";
        case ERR_ADDRESS_IN_USE_NET: return "Network address already in use";

        /* Kernel */
        case ERR_PANIC: return "Kernel panic";
        case ERR_INTERNAL: return "Internal kernel error";

        default: return "Unrecognized error code";
    }
}