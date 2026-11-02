#include "kernel/fs/ramfs.h"
#include "kernel/mm/heap.h"
#include "libc/string.h"
#include "libc/types.h"
#include "libc/mem.h"
#include "libc/errno.h"
#include "ramfs_internal.h"

static int ramfs_mount_fn(struct mount *mp, const char *opts);
static int ramfs_unmount_fn(struct mount *mp);
static int ramfs_sync_fn(struct mount *mp);
static int ramfs_root_vnode(struct mount *mp, struct vnode **out_root);

const struct vnode_ops ramfs_vnode_ops = {
    .lookup = ramfs_lookup_vn,
    .create = ramfs_create_vn,
    .unlink = ramfs_unlink_vn,
    .read = ramfs_read_vn,
    .write = ramfs_write_vn,
    .readdir = ramfs_readdir_vn,
    .truncate = ramfs_truncate_vn,
    .getattr = ramfs_getattr_vn,
    .setattr = ramfs_setattr_vn,
    .destroy = ramfs_destroy_vnode,
};

static const struct fs_ops ramfs_fs_ops = {
    .mount = ramfs_mount_fn,
    .unmount = ramfs_unmount_fn,
    .sync = ramfs_sync_fn,
};

struct fs_type ramfs_type = {
    .name = "ramfs",
    .fs_ops = &ramfs_fs_ops,
    .root_vnode = ramfs_root_vnode,
};

void ramfs_init(void)
{
    vfs_register_fs(&ramfs_type);
}

struct ramfs_node *ramfs_node_create(const char *name, vnode_type_t type)
{
    if (!name)
        return NULL;

    struct ramfs_node *node = calloc(1, sizeof(*node));
    if (!node)
        return NULL;

    usize name_len = strlen(name);
    node->name = malloc(name_len + 1);
    if (!node->name)
    {
        free(node);
        return NULL;
    }

    memcpy(node->name, name, name_len + 1);
    node->type = type;
    return node;
}

struct vnode *ramfs_node_vnode(struct mount *mp, struct ramfs_node *node)
{
    if (!node)
        return NULL;

    if (!node->vnode_cache)
    {
        node->vnode_cache = vnode_create(mp, node->type, &ramfs_vnode_ops, node);
        if (!node->vnode_cache)
            return NULL;
    }

    return node->vnode_cache;
}

void ramfs_node_free(struct ramfs_node *node)
{
    if (!node)
        return;

    free(node->data);
    free(node->name);
    free(node);
}

void ramfs_node_release(struct ramfs_node *node)
{
    if (!node)
        return;

    if (node->vnode_cache)
    {
        struct vnode *vn = node->vnode_cache;
        node->vnode_cache = NULL;
        vnode_unref(vn);
    }
    else
    {
        ramfs_node_free(node);
    }
}

void ramfs_tree_release(struct ramfs_node *node)
{
    if (!node)
        return;

    while (node->children)
    {
        struct ramfs_node *child = node->children;
        node->children = child->sibling;
        child->sibling = NULL;
        child->parent = NULL;
        ramfs_tree_release(child);
    }

    ramfs_node_release(node);
}

void ramfs_destroy_vnode(struct vnode *vn)
{
    if (!vn)
        return;

    struct ramfs_node *node = vn->fs_data;
    if (node)
    {
        if (node->vnode_cache == vn)
            node->vnode_cache = NULL;
        vn->fs_data = NULL;
        ramfs_node_free(node);
    }
}

static int ramfs_mount_fn(struct mount *mp, const char *opts)
{
    (void)opts;
    if (!mp)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_mount *rm = calloc(1, sizeof(*rm));
    if (!rm)
        return ERR_OUT_OF_MEMORY;

    rm->root = ramfs_node_create("", VNODE_TYPE_DIR);
    if (!rm->root)
    {
        free(rm);
        return ERR_OUT_OF_MEMORY;
    }

    mp->fs_data = rm;
    return ERR_SUCCESS;
}

static int ramfs_unmount_fn(struct mount *mp)
{
    if (!mp)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_mount *rm = mp->fs_data;
    if (!rm)
        return ERR_SUCCESS;

    mp->fs_data = NULL;
    ramfs_tree_release(rm->root);
    free(rm);
    return ERR_SUCCESS;
}

static int ramfs_sync_fn(struct mount *mp)
{
    return mp ? ERR_SUCCESS : ERR_INVALID_ARGUMENT;
}

static int ramfs_root_vnode(struct mount *mp, struct vnode **out_root)
{
    if (!mp || !out_root)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_mount *rm = mp->fs_data;
    if (!rm || !rm->root)
        return ERR_INVALID_ARGUMENT;

    *out_root = ramfs_node_vnode(mp, rm->root);
    return *out_root ? ERR_SUCCESS : ERR_OUT_OF_MEMORY;
}
