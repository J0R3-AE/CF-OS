#include "kernel/fs/vfs.h"
#include "vfs_internal.h"
#include "libc/errno.h"
#include "libc/mem.h"
#include "libc/string.h"
#include "kernel/mm/heap.h"

#define VFS_NAME_MAX 255

static int vfs_next_component(const char **cursor, char *name, bool *has_component,
                              bool *has_more)
{
    const char *p = *cursor;
    while (*p == '/')
        ++p;

    *has_component = false;
    *has_more = false;
    if (!*p)
    {
        *cursor = p;
        return ERR_SUCCESS;
    }

    usize len = 0;
    while (p[len] && p[len] != '/')
    {
        if (len == VFS_NAME_MAX)
            return ERR_NAME_TOO_LONG;
        name[len] = p[len];
        ++len;
    }
    name[len] = '\0';

    p += len;
    while (*p == '/')
        ++p;

    *has_component = true;
    *has_more = *p != '\0';
    *cursor = p;
    return ERR_SUCCESS;
}

static int vfs_lookup_component(struct vnode *dir, const char *name, struct vnode **out)
{
    if (!dir || !name || !out)
        return ERR_INVALID_ARGUMENT;
    if (dir->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;
    if (!dir->ops || !dir->ops->lookup)
        return ERR_NOT_SUPPORTED;

    return dir->ops->lookup(dir, name, out);
}

int vfs_lookup(const char *path, struct vnode **out)
{
    if (!path || !path[0] || !out)
        return ERR_INVALID_ARGUMENT;

    *out = NULL;
    struct vnode *current = vfs_get_root();
    if (!current)
        return ERR_NOT_FOUND;

    const char *cursor = path;
    char name[VFS_NAME_MAX + 1];
    bool has_component;
    bool has_more;

    for (;;)
    {
        int result = vfs_next_component(&cursor, name, &has_component, &has_more);
        if (result != ERR_SUCCESS)
            return result;
        if (!has_component)
            break;

        struct vnode *next = NULL;
        result = vfs_lookup_component(current, name, &next);
        if (result != ERR_SUCCESS)
            return result;
        if (!next)
            return ERR_INTERNAL;
        current = next;
    }

    usize path_len = strlen(path);
    if (path_len && path[path_len - 1] == '/' && current->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;

    *out = current;
    return ERR_SUCCESS;
}

struct vnode *vfs_resolve(const char *path)
{
    struct vnode *vn = NULL;
    return vfs_lookup(path, &vn) == ERR_SUCCESS ? vn : NULL;
}

int vfs_resolve_parent_path(const char *path, struct vnode **parent_out,
                            char **leaf_name_out)
{
    if (!path || !path[0] || !parent_out || !leaf_name_out)
        return ERR_INVALID_ARGUMENT;

    *parent_out = NULL;
    *leaf_name_out = NULL;

    struct vnode *current = vfs_get_root();
    if (!current)
        return ERR_NOT_FOUND;

    const char *cursor = path;
    char name[VFS_NAME_MAX + 1];
    bool has_component;
    bool has_more;

    for (;;)
    {
        int result = vfs_next_component(&cursor, name, &has_component, &has_more);
        if (result != ERR_SUCCESS)
            return result;
        if (!has_component)
            return ERR_INVALID_ARGUMENT;

        if (!has_more)
        {
            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                return ERR_INVALID_ARGUMENT;

            usize len = strlen(name);
            char *leaf = malloc(len + 1);
            if (!leaf)
                return ERR_OUT_OF_MEMORY;
            memcpy(leaf, name, len + 1);

            *parent_out = current;
            *leaf_name_out = leaf;
            return ERR_SUCCESS;
        }

        struct vnode *next = NULL;
        result = vfs_lookup_component(current, name, &next);
        if (result != ERR_SUCCESS)
            return result;
        if (!next)
            return ERR_INTERNAL;
        current = next;
    }
}

struct vnode *vfs_resolve_path(const char *path, struct vnode **parent_out,
                               const char **leaf_name_out)
{
    if (!parent_out || !leaf_name_out)
        return NULL;

    char *leaf = NULL;
    int result = vfs_resolve_parent_path(path, parent_out, &leaf);
    if (result != ERR_SUCCESS)
        return NULL;

    *leaf_name_out = leaf;
    return *parent_out;
}
