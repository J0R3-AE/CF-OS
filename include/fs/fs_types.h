#pragma once
/**
 * @file fs_types.h
 * @brief Core filesystem types: vnode, file, operations, and enums.
 *
 * This header defines the fundamental structures used by all filesystems:
 * - vnode types and operations
 * - file operations
 * - filesystem mount operations
 * - exec function type
 *
 * These types form the backbone of the VFS layer and are implemented by
 * individual filesystem backends (ramfs, ext2, devfs, etc.).
 */

#include <stddef.h>
#include <stdint.h>
#include "libk/types.h"

struct vnode;
struct file;
struct mount;
struct fs_type;
/* -------------------------------------------------------------------------- */
/*  Vnode Types                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @enum vnode_type_t
 * @brief Basic vnode types supported by the VFS.
 */
typedef enum
{
    VNODE_TYPE_NONE = 0, /**< Uninitialized or invalid vnode. */
    VNODE_TYPE_FILE,     /**< Regular file. */
    VNODE_TYPE_DIR,      /**< Directory. */
    VNODE_TYPE_DEV,      /**< Character device. */
    VNODE_TYPE_BLOCK,    /**< Block device. */
    VNODE_TYPE_EXEC      /**< Executable node (function‑backed). */
} vnode_type_t;

/**
 * @brief Function pointer type for executable vnodes.
 *
 * Executable vnodes behave like tiny userland programs stored in the VFS.
 */
typedef void (*exec_fn_t)(const char *args);

/* -------------------------------------------------------------------------- */
/*  Vnode Operations                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @struct vnode_ops
 * @brief Per‑vnode operation table implemented by each filesystem.
 *
 * These operations define how a filesystem handles:
 * - directory traversal (lookup, create, unlink, readdir)
 * - file I/O (read, write, truncate)
 * - metadata (getattr, setattr)
 */
struct vnode_ops
{
    int (*lookup)(struct vnode *dir, const char *name, struct vnode **out);
    int (*create)(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out);
    int (*link)(struct vnode *dir, const char *name, struct vnode *target);
    int (*unlink)(struct vnode *dir, const char *name);

    int (*read)(struct vnode *vn, void *buf, usize off, usize len, usize *out);
    int (*write)(struct vnode *vn, const void *buf, usize off, usize len, usize *out);
    int (*readdir)(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out);
    int (*writedir)(struct vnode *vn, const char *name, vnode_type_t type);
    int (*truncate)(struct vnode *vn, usize new_size);
    int (*getattr)(struct vnode *vn, void *stat_buf);
    int (*setattr)(struct vnode *vn, const void *stat_buf);
};

/* -------------------------------------------------------------------------- */
/*  Filesystem Mount Operations                                               */
/* -------------------------------------------------------------------------- */

/**
 * @struct fs_ops
 * @brief Filesystem‑level mount/unmount/sync operations.
 *
 * Each filesystem provides these to integrate with the VFS mount layer.
 */
struct fs_ops
{
    int (*mount)(struct mount *mp, const char *opts);
    int (*unmount)(struct mount *mp);
    int (*sync)(struct mount *mp);
};

/* -------------------------------------------------------------------------- */
/*  File Operations                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @struct file_ops
 * @brief Per‑file operation table.
 *
 * These operations act on an open file handle, not the vnode itself.
 */
struct file_ops
{
    int (*read)(struct file *f, void *buf, usize len, usize *out);
    int (*write)(struct file *f, const void *buf, usize len, usize *out);
    int (*seek)(struct file *f, usize off);
    int (*close)(struct file *f);
};

/* -------------------------------------------------------------------------- */
/*  File Structure                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @struct file
 * @brief Represents an open file handle.
 *
 * Contains:
 * - current offset
 * - flags
 * - file operations
 * - filesystem‑specific private data
 * - reference count
 */
struct file
{
    struct vnode *vn;           /**< Associated vnode. */
    usize offset;               /**< Current read/write offset. */
    u32 flags;                  /**< Open flags (VFS_O_*). */
    const struct file_ops *ops; /**< File operation table. */
    void *priv;                 /**< Filesystem‑specific data. */
    int refcount;               /**< Reference count. */
};

/* -------------------------------------------------------------------------- */
/*  Vnode Structure                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @struct vnode
 * @brief Represents a filesystem object (file, directory, device, etc.).
 *
 * Vnodes are the core abstraction of the VFS. Each vnode contains:
 * - type
 * - reference count
 * - pointer to its mount
 * - operation table
 * - filesystem‑specific data
 * - optional exec handler for VNODE_TYPE_EXEC
 */
struct vnode
{
    vnode_type_t type;           /**< Vnode type. */
    u32 refcnt;                  /**< Reference count. */
    struct mount *mount;         /**< Mount this vnode belongs to. */
    const struct vnode_ops *ops; /**< Operation table. */
    void *fs_data;               /**< Filesystem‑specific data. */

    exec_fn_t exec; /**< Executable handler (optional). */
};

/* FS_TYPES_H */
