#pragma once
/**
 * @file vfs.h
 * @brief Virtual File System (VFS) core interfaces and filesystem registration.
 *
 * The VFS provides a unified abstraction layer over multiple filesystem types.
 * It handles:
 * - filesystem type registration
 * - mounting/unmounting
 * - path resolution
 * - vnode and file operations
 * - open/read/write/close
 *
 * Filesystems register themselves via `struct fs_type`, providing their
 * operations and mount handlers. The VFS then routes all file operations
 * through the appropriate filesystem backend.
 */

#include "fs_types.h"
#include "libk/link.h"

/* -------------------------------------------------------------------------- */
/*  Flags & Error Codes                                                       */
/* -------------------------------------------------------------------------- */

/** @brief Open for read‑only access. */
#define VFS_O_RDONLY 0x1
/** @brief Open for write‑only access. */
#define VFS_O_WRONLY 0x2
/** @brief Open for read/write access. */
#define VFS_O_RDWR (VFS_O_RDONLY | VFS_O_WRONLY)
/** @brief Create file if it does not exist. */
#define VFS_O_CREATE 0x4

/** @brief Operation succeeded. */
#define EOK 0
/** @brief No such file or directory. */
#define ENOENT -1
/** @brief Invalid argument. */
#define EINVAL -2
/** @brief Permission denied. */
#define EACCES -3
/** @brief Out of memory. */
#define ENOMEM -4
/** @brief Filesystem‑specific error. */
#define EFS -5

struct vnode;
struct file;
struct mount;
struct fs_type;

/* -------------------------------------------------------------------------- */
/*  Filesystem Type Registration                                              */
/* -------------------------------------------------------------------------- */

/**
 * @struct fs_type
 * @brief Represents a filesystem implementation registered with the VFS.
 *
 * Each filesystem type provides:
 * - a unique name
 * - a table of filesystem operations (`fs_ops`)
 * - an optional helper to retrieve the root vnode for a mount
 */
struct fs_type
{
    Link link;
    const char *name;            /**< Filesystem name (e.g. "ramfs"). */
    const struct fs_ops *fs_ops; /**< Filesystem operation table. */
    int (*root_vnode)(struct mount *mp,
                      struct vnode **out_root); /**< Optional root vnode helper. */
};

typedef struct vfs_stat
{
    vnode_type_t type;

    usize size;      // file size
    usize capacity;  // allocated (optional but useful for ramfs)

    u32 mode;        // permissions (future use)
    u32 uid;
    u32 gid;

    u32 atime;
    u32 mtime;
    u32 ctime;
} vfs_stat_t;

/* -------------------------------------------------------------------------- */
/*  VFS Core API                                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Register a filesystem type with the VFS.
 *
 * @param fst Filesystem type descriptor.
 * @return 0 on success, -1 on duplicate name or failure.
 */
int vfs_register_fs(struct fs_type *fst);

/**
 * @brief Mount a filesystem.
 *
 * @param fs_name Filesystem type name.
 * @param source  Source string (device, image, etc.).
 * @param target  Target mount point.
 * @param opts    Optional mount options.
 * @return 0 on success, -1 on failure.
 */
int vfs_mount(const char *fs_name, const char *source,
              const char *target, const char *opts);

/**
 * @brief Unmount a filesystem.
 *
 * @param target Target mount point.
 * @return 0 on success, -1 on failure.
 */
int vfs_unmount(const char *target);

/* -------------------------------------------------------------------------- */
/*  File Operations                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Open a file by path.
 *
 * @param path File path.
 * @param flags Open flags (VFS_O_*).
 * @param out Output file handle.
 * @return 0 on success, -1 on failure.
 */
int vfs_open(const char *path, u32 flags, struct file **out);

/**
 * @brief Open a vnode directly (bypassing path lookup).
 *
 * @param vn Vnode to open.
 * @return File object or NULL on failure.
 */
struct file *vfs_open_vnode(struct vnode *vn);

/**
 * @brief Close a file.
 *
 * @param f File handle.
 * @return 0 on success, -1 on failure.
 */
int vfs_close(struct file *f);

/**
 * @brief Read from a file.
 *
 * @param f File handle.
 * @param buf Output buffer.
 * @param len Maximum bytes to read.
 * @param out Number of bytes actually read.
 * @return 0 on success, -1 on failure.
 */
int vfs_read(struct file *f, void *buf, usize len, usize *out);

/**
 * @brief Write to a file.
 *
 * @param f File handle.
 * @param buf Input buffer.
 * @param len Number of bytes to write.
 * @param out Number of bytes actually written.
 * @return 0 on success, -1 on failure.
 */
int vfs_write(struct file *f, const void *buf, usize len, usize *out);

/* -------------------------------------------------------------------------- */
/*  Path Resolution                                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a path to a vnode.
 *
 * @param path Path string.
 * @param out Output vnode.
 * @return 0 on success, -1 on failure.
 */
int vfs_lookup(const char *path, struct vnode **out);

/**
 * @brief Change the root vnode for path resolution.
 *
 * @param new_root New root vnode.
 * @return 0 on success, -1 on failure.
 */
int vfs_chroot(struct vnode *new_root);

/**
 * @brief Set the global root vnode (used during root filesystem mount).
 *
 * @param root Root vnode.
 * @return 0 on success, -1 on failure.
 */
int vfs_set_root(struct vnode *root);

/**
 * @brief Get the current root vnode.
 *
 * @return Root vnode or NULL.
 */
struct vnode *vfs_get_root(void);

/**
 * @brief Resolve a path and return the vnode directly.
 *
 * @param path Path string.
 * @return Vnode or NULL on failure.
 */
struct vnode *vfs_resolve(const char *path);

/**
 * @brief Resolve a path and return parent vnode + leaf name.
 *
 * Useful for create/unlink operations.
 *
 * @param path Path string.
 * @param parent_out Output parent vnode.
 * @param leaf_name_out Output leaf name.
 * @return Vnode or NULL on failure.
 */
struct vnode *vfs_resolve_path(const char *path,
                               struct vnode **parent_out,
                               const char **leaf_name_out);

/* -------------------------------------------------------------------------- */
/*  Exec Helpers                                                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief Create an executable vnode backed by a function pointer.
 *
 * @param path Path to create.
 * @param fn   Function to execute.
 * @return 0 on success, -1 on failure.
 */
int vfs_create_exec(const char *path, exec_fn_t fn);

/* -------------------------------------------------------------------------- */
/*  File Reference Management                                                 */
/* -------------------------------------------------------------------------- */

/// @brief Increment file reference count.
void file_ref(struct file *f);

/// @brief Decrement file reference count and free when it reaches zero.
void file_unref(struct file *f);
/* VFS_H */
