#pragma once
/**
 * @file ramfs.h
 * @brief In‑memory RAM filesystem (ramfs) implementation.
 *
 * ramfs is a simple, non‑persistent filesystem stored entirely in memory.
 * It supports:
 * - hierarchical directories
 * - file creation, deletion, read/write
 * - dynamic resizing of file data
 * - optional device hooks for vnode-backed devices
 *
 * This filesystem is ideal for early boot, temporary storage, and testing.
 */

#include "fs_types.h"
#include "vfs.h"
#include "mount.h"

/* -------------------------------------------------------------------------- */
/*  RAMFS Node                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @struct ramfs_node
 * @brief Internal representation of a ramfs file or directory.
 *
 * Nodes form a tree:
 * - `parent` points upward
 * - `children` is the head of a linked list of child nodes
 * - `sibling` links nodes within the same directory
 *
 * Files store data in a dynamically resized buffer.
 * Directories store only metadata and child pointers.
 */
struct ramfs_node
{
    char *name;        /**< Node name. */
    vnode_type_t type; /**< File or directory. */
    struct vnode *vnode_cache;

    struct ramfs_node *parent;   /**< Parent directory. */
    struct ramfs_node *sibling;  /**< Next node in directory. */
    struct ramfs_node *children; /**< First child (for directories). */

    /* File data */
    u8 *data;       /**< File contents. */
    usize size;     /**< Current file size. */
    usize capacity; /**< Allocated buffer size. */

    /* Optional device hooks */
    int (*dev_read)(void *buf, usize len);
    int (*dev_write)(const void *buf, usize len);
};

/* -------------------------------------------------------------------------- */
/*  RAMFS Mount                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @struct ramfs_mount
 * @brief Per‑mount ramfs state.
 */
struct ramfs_mount
{
    struct ramfs_node *root; /**< Root directory node. */
    struct vnode *root_vnode_cache; /**< Cached root vnode for this mount. */
};

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

/// @brief Global filesystem type descriptor for ramfs.
extern struct fs_type ramfs_type;

/**
 * @brief Initialize and register the ramfs filesystem type.
 *
 * Call during kernel initialization.
 */
void ramfs_init(void);

/**
 * @brief Assign device read/write hooks to a vnode.
 *
 * @param vn        Target vnode.
 * @param dev_read  Device read callback.
 * @param dev_write Device write callback.
 */
void ramfs_set_dev_hooks(struct vnode *vn, int (*dev_read)(void *buf, usize len),
                         int (*dev_write)(const void *buf, usize len));

/* -------------------------------------------------------------------------- */
/*  Internal FS Operations (static, not exported)                             */
/* -------------------------------------------------------------------------- */
/* These remain static in the .c file and intentionally undocumented here.    */

static int ramfs_mount_fn(struct mount *mp, const char *opts);
static int ramfs_unmount_fn(struct mount *mp);
static int ramfs_sync_fn(struct mount *mp);
static int ramfs_root_vnode(struct mount *mp, struct vnode **out_root);
static int ramfs_lookup_vn(struct vnode *dir, const char *name, struct vnode **out);
static int ramfs_create_vn(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out);
static int ramfs_unlink_vn(struct vnode *dir, const char *name);
static int ramfs_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out);
static int ramfs_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out);
static int ramfs_readdir_vn(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out);
static int ramfs_truncate_vn(struct vnode *vn, usize new_size);
static int ramfs_getattr_vn(struct vnode *vn, void *stat_buf);

/* RAMFS_H */
