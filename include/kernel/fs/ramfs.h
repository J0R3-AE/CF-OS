#pragma once
/**
 * @file ramfs.h
 * @brief In‑memory RAM filesystem (ramfs) implementation.
 *
 * ramfs is a simple, non‑persistent filesystem stored entirely in memory.
 * It supports:
 * - hierarchical directories
 * - file creation, deletion, read/write, and truncate
 * - dynamic resizing and zero-filled growth of file data
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

/*
 * Extract a ustar-format tarball into the given ramfs root vnode.
 * The tarball should be a contiguous memory blob (start,size).
 */
void tar_extract(struct vnode *root, void *start, u32 size);

/* RAMFS_H */
