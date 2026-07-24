#pragma once
/**
 * @file fd.h
 * @brief File descriptor table and descriptor management helpers.
 *
 * Provides a simple per‑process file descriptor table. Each entry maps an
 * integer file descriptor (fd) to an open `struct file`. This layer sits
 * above the VFS and handles:
 *
 * - allocating new file descriptors
 * - closing descriptors
 * - duplicating descriptors (dup/dup2)
 * - retrieving file objects by fd
 *
 * The implementation is intentionally minimal and can be extended later with:
 * - per‑fd flags
 * - CLOEXEC
 * - soft/hard fd limits
 * - per‑process metadata
 */

#include "fs_types.h"
#include "vfs.h"
#include "mount.h"
#include <stdbool.h>

#ifndef FD_MAX
/** @brief Maximum number of file descriptors per process. */
#define FD_MAX 256
#endif

/* -------------------------------------------------------------------------- */
/*  File Descriptor Entry                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @struct fd_entry
 * @brief Represents a single file descriptor slot.
 *
 * Each entry contains:
 * - a pointer to an open file object
 * - a `used` flag for quick allocation checks
 */
typedef struct fd_entry
{
    struct file *file; /**< File object, or NULL if unused. */
    bool used;         /**< Whether this descriptor slot is in use. */
} fd_entry_t;

/* -------------------------------------------------------------------------- */
/*  File Descriptor Table                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @struct fd_table
 * @brief Per‑process file descriptor table.
 *
 * Contains a fixed‑size array of descriptor entries indexed by fd number.
 */
typedef struct fd_table
{
    fd_entry_t entries[FD_MAX]; /**< Descriptor entries. */
} fd_table_t;

/* -------------------------------------------------------------------------- */
/*  FD Table Management API                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize a file descriptor table.
 *
 * Marks all entries as unused.
 */
void fd_table_init(fd_table_t *t);

/**
 * @brief Allocate a new file descriptor for a file.
 *
 * @param t File descriptor table.
 * @param f File object to associate.
 * @return New fd on success, -1 on failure.
 */
int fd_alloc(fd_table_t *t, struct file *f);

/**
 * @brief Allocate a specific fd number.
 *
 * @param t       File descriptor table.
 * @param want_fd Desired fd index.
 * @param f       File object.
 * @return Allocated fd on success, -1 on failure.
 */
int fd_alloc_at(fd_table_t *t, int want_fd, struct file *f);

/**
 * @brief Retrieve the file object for a given fd.
 *
 * @param t  File descriptor table.
 * @param fd Descriptor number.
 * @return File pointer or NULL on failure.
 */
struct file *fd_get(fd_table_t *t, int fd);

/**
 * @brief Close a file descriptor and free its slot.
 *
 * @param t  File descriptor table.
 * @param fd Descriptor number.
 * @return 0 on success, -1 on failure.
 */
int fd_close(fd_table_t *t, int fd);

/**
 * @brief Duplicate a file descriptor to a new free slot.
 *
 * @param t     File descriptor table.
 * @param oldfd Descriptor to duplicate.
 * @return New fd on success, -1 on failure.
 */
int fd_dup(fd_table_t *t, int oldfd);

/**
 * @brief Duplicate a file descriptor into a specific slot.
 *
 * @param t      File descriptor table.
 * @param oldfd  Existing descriptor.
 * @param newfd  Target descriptor.
 * @return newfd on success, -1 on failure.
 */
int fd_dup2(fd_table_t *t, int oldfd, int newfd);

/**
 * @brief Install a file into a new fd and take ownership.
 *
 * Caller should not free the file afterward.
 *
 * @param t File descriptor table.
 * @param f File object.
 * @return New fd on success, -1 on failure.
 */
int fd_install_take(fd_table_t *t, struct file *f);

/**
 * @brief Count the number of allocated descriptors.
 *
 * @param t File descriptor table.
 * @return Count on success, -1 on invalid table.
 */
int fd_count(fd_table_t *t);

/* FS_FD_H */
