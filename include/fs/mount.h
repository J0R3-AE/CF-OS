#pragma once
/**
 * @file mount.h
 * @brief Kernel mount table and filesystem mount management.
 *
 * This subsystem provides a minimal mount interface for the VFS. It supports:
 * - mounting a filesystem type at a target path
 * - retrieving the root vnode of a mount
 * - listing mounts
 * - unmounting
 *
 * The implementation is intentionally simple: a linked list of mounts.
 * It can be extended later to support multiple mount points, options,
 * lazy unmount, and more advanced VFS semantics.
 */

#include "fs_types.h"
#include "vfs.h"
#include "libk/link.h"

/* -------------------------------------------------------------------------- */
/*  Mount Structure                                                            */
/* -------------------------------------------------------------------------- */

/**
 * @struct mount
 * @brief Represents a mounted filesystem instance.
 *
 * Each mount links a filesystem type (fs_type), a source (device or image),
 * and a target path (e.g. "/"). Filesystem-specific data (fs_data) is stored
 * here as well, such as superblocks or device handles.
 */
struct mount
{
    Link link;                /**< Intrusive node for the global mount list. */
    struct fs_type *type;     /**< Filesystem type (e.g. ramfs, ext2). */
    struct vnode *root_vnode; /**< Root vnode for this mount. */
    char *source;             /**< Source string (device, image, etc.). */
    char *target;             /**< Target mount point (e.g. "/"). */
    void *fs_data;            /**< Filesystem-specific data. */
};

/**
 * @struct mount_stat
 * @brief Lightweight mount information for user queries.
 */
struct mount_stat
{
    const char *source; /**< Source string (device, image, etc.). */
    const char *target; /**< Target mount point. */
    const char *type;   /**< Filesystem type name. */
};

/* -------------------------------------------------------------------------- */
/*  Mount API                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Mount a filesystem.
 *
 * @param fst     Filesystem type.
 * @param source  Source string (device, image, etc.).
 * @param target  Target mount point (e.g. "/").
 * @param opts    Optional mount options.
 * @return 0 on success, -1 on failure.
 */
int mount_do_mount(struct fs_type *fst, const char *source,
                   const char *target, const char *opts);

/**
 * @brief Unmount a filesystem.
 *
 * @param target Target mount point.
 * @return 0 on success, -1 on failure.
 */
int mount_do_unmount(const char *target);

/**
 * @brief Look up a mount by target path.
 *
 * @param target     Target mount point.
 * @param out_mount  Output pointer to the found mount.
 * @return 0 on success, -1 if not found.
 */
int mount_lookup(const char *target, struct mount **out_mount);

/**
 * @brief Look up a mount and return its root vnode.
 *
 * @param target     Target mount point.
 * @param out_vnode  Output pointer to the root vnode.
 * @return 0 on success, -1 if not found.
 */
int mount_lookup_vnode(const char *target, struct vnode **out_vnode);

/**
 * @brief Return a list of all mounts.
 *
 * @param out_mounts Output: array of mount pointers.
 * @param out_count  Output: number of mounts.
 * @return 0 on success, -1 on failure.
 *
 * @note Caller must free the array, but not the mounts themselves.
 */
int mount_list(struct mount ***out_mounts, usize *out_count);

/**
 * @brief Clean up all mounts (optional).
 *
 * @return 0 on success, -1 on failure.
 */
int mount_cleanup(void);

/**
 * @brief Initialize the mount subsystem.
 *
 * @return 0 on success, -1 on failure.
 */
int mount_init(void);

/**
 * @brief Shut down the mount subsystem.
 *
 * @return 0 on success, -1 on failure.
 */
int mount_shutdown(void);

/**
 * @brief Sync all mounted filesystems (optional).
 *
 * @return 0 on success, -1 on failure.
 */
int mount_sync(void);

/**
 * @brief Retrieve mount information for a specific target.
 *
 * @param target    Target mount point.
 * @param out_stat  Output mount information.
 * @return 0 on success, -1 if not found.
 */
int mount_stat(const char *target, struct mount_stat *out_stat);

/* MOUNT_H */