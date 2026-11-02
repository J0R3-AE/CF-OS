#include "ramfs_internal.h"
#include "libc/errno.h"
#include "libc/string.h"

static struct ramfs_node *ramfs_find_child(struct ramfs_node *dir, const char *name)
{
    for (struct ramfs_node *child = dir->children; child; child = child->sibling)
    {
        if (strcmp(child->name, name) == 0)
            return child;
    }

    return NULL;
}

int ramfs_lookup_vn(struct vnode *dir, const char *name, struct vnode **out)
{
    if (!dir || !name || !out)
        return ERR_INVALID_ARGUMENT;
    if (dir->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;

    *out = NULL;
    struct ramfs_node *node = dir->fs_data;
    if (!node)
        return ERR_INVALID_ARGUMENT;

    if (strcmp(name, ".") == 0)
    {
        *out = dir;
        return ERR_SUCCESS;
    }

    if (strcmp(name, "..") == 0)
    {
        struct ramfs_node *parent = node->parent ? node->parent : node;
        *out = ramfs_node_vnode(dir->mount, parent);
        return *out ? ERR_SUCCESS : ERR_OUT_OF_MEMORY;
    }

    struct ramfs_node *child = ramfs_find_child(node, name);
    if (!child)
        return ERR_NOT_FOUND;

    *out = ramfs_node_vnode(dir->mount, child);
    return *out ? ERR_SUCCESS : ERR_OUT_OF_MEMORY;
}

int ramfs_create_vn(struct vnode *dir, const char *name, vnode_type_t type, struct vnode **out)
{
    if (!dir || !name || !out)
        return ERR_INVALID_ARGUMENT;
    if (dir->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;
    if (!name[0] || strchr(name, '/') || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return ERR_INVALID_ARGUMENT;
    if (type != VNODE_TYPE_FILE && type != VNODE_TYPE_DIR &&
        type != VNODE_TYPE_DEV && type != VNODE_TYPE_EXEC)
        return ERR_NOT_SUPPORTED;

    *out = NULL;
    struct ramfs_node *parent = dir->fs_data;
    if (!parent)
        return ERR_INVALID_ARGUMENT;
    if (ramfs_find_child(parent, name))
        return ERR_ALREADY_EXISTS;

    struct ramfs_node *child = ramfs_node_create(name, type);
    if (!child)
        return ERR_OUT_OF_MEMORY;

    child->parent = parent;
    struct vnode *vn = ramfs_node_vnode(dir->mount, child);
    if (!vn)
    {
        child->parent = NULL;
        ramfs_node_release(child);
        return ERR_OUT_OF_MEMORY;
    }

    child->sibling = parent->children;
    parent->children = child;
    *out = vn;
    return ERR_SUCCESS;
}

int ramfs_unlink_vn(struct vnode *dir, const char *name)
{
    if (!dir || !name)
        return ERR_INVALID_ARGUMENT;
    if (dir->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;
    if (!name[0] || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node *parent = dir->fs_data;
    if (!parent)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node **link = &parent->children;
    while (*link)
    {
        struct ramfs_node *child = *link;
        if (strcmp(child->name, name) == 0)
        {
            if (child->type == VNODE_TYPE_DIR && child->children)
                return ERR_DIRECTORY_NOT_EMPTY;

            *link = child->sibling;
            child->parent = NULL;
            child->sibling = NULL;
            ramfs_node_release(child);
            return ERR_SUCCESS;
        }

        link = &child->sibling;
    }

    return ERR_NOT_FOUND;
}

int ramfs_readdir_vn(struct vnode *vn, usize index, const char **name_out,
                     vnode_type_t *type_out)
{
    if (!vn)
        return ERR_INVALID_ARGUMENT;
    if (vn->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;

    struct ramfs_node *dir = vn->fs_data;
    if (!dir)
        return ERR_INVALID_ARGUMENT;

    usize current = 0;
    for (struct ramfs_node *child = dir->children; child; child = child->sibling)
    {
        if (current++ != index)
            continue;

        if (name_out)
            *name_out = child->name;
        if (type_out)
            *type_out = child->type;
        return ERR_SUCCESS;
    }

    return ERR_NOT_FOUND;
}
