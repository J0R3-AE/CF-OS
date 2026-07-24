#pragma once

/**
 *  @file vnode.h
 *  @brief Virtual node abstraction.
 */

#include "libc/types.h"

struct vnode;
struct mount;
// Vnode Types
typedef enum
{
    VNODE_TYPE_NONE = 0,
    VNODE_TYPE_FILE,
    VNODE_TYPE_DIR,
    VNODE_TYPE_DEV,
    VNODE_TYPE_BLOCK,
    VNODE_TYPE_EXEC
} vnode_type_t;

// Executable vnode callback
typedef void (*exec_fn_t)(const char *args);

// Vnode Operations
struct vnode_ops
{
    // directory operations */
    int (*lookup)(struct vnode *dir, const char *name, struct vnode **out);
    int (*create)(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out);
    int (*link)(struct vnode *dir, const char *name, struct vnode *target);
    int (*unlink)(struct vnode *dir, const char *name);

    // file operations */
    int (*read)(struct vnode *vn, void *buf, usize off, usize len, usize *out);
    int (*write)(struct vnode *vn, const void *buf, usize off, usize len, usize *out);

    // directory reading */
    int (*readdir)(struct vnode *vn, usize index, const char **name_out, vnode_type_t *type_out);
    int (*writedir)(struct vnode *vn, const char *name, vnode_type_t type);

    // metadata */
    int (*truncate)(struct vnode *vn, usize new_size);
    int (*getattr)(struct vnode *vn, void *stat_buf);
    int (*setattr)(struct vnode *vn, const void *stat_buf);

    void (*destroy)(struct vnode *);
};

// Vnode
struct vnode
{
    vnode_type_t type;
    u32 refcnt;
    struct mount *mount;
    const struct vnode_ops *ops;
    void *fs_data;
    exec_fn_t exec;
};

struct vnode *vnode_create(struct mount *mp, vnode_type_t type, const struct vnode_ops *ops, void *fs_data);

void vnode_ref(struct vnode *vn);
void vnode_unref(struct vnode *vn);