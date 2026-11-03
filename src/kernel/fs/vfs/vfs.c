#include "kernel/fs/vfs.h"
#include "kernel/fs/fs_types.h"
#include "kernel/fs/mount.h"
<<<<<<< HEAD
=======
#include "kernel/mm/heap.h"
#include "kernel/proc/proc.h"
#include "vfs_internal.h"
#include "libc/errno.h"
#include "kernel/list.h"
>>>>>>> 0314c3a (	modified:   Makefile)
#include "libc/string.h"
#include "libc/mem.h"
#include "libc/link.h"
#include "libc/errno.h"
#include "kernel/mm/heap.h"
#include "kernel/proc/exec.h"
#include "vfs_internal.h"

extern const struct file_ops vfs_generic_file_ops;

Link g_fs_types;
static struct vnode *g_root_vnode;

int vfs_register_fs(struct fs_type *fst)
{
    if (!fst || !fst->name || !fst->name[0])
        return ERR_INVALID_ARGUMENT;

    struct fs_type *registered;
    ListForEachEntry(registered, g_fs_types, link)
    {
        if (strcmp(registered->name, fst->name) == 0)
            return ERR_ALREADY_EXISTS;
    }

    LinkInit(&fst->link);
    LinkBefore(&g_fs_types, &fst->link);
    return ERR_SUCCESS;
}

static struct fs_type *find_fs_type(const char *name)
{
    if (!name)
        return NULL;

    struct fs_type *registered;
    ListForEachEntry(registered, g_fs_types, link)
    {
        if (strcmp(registered->name, name) == 0)
            return registered;
    }

    return NULL;
}

int vfs_set_root(struct vnode *root)
{
    g_root_vnode = root;
    return ERR_SUCCESS;
}

struct vnode *vfs_get_root(void)
{
    return g_root_vnode;
}

int vfs_open(const char *path, u32 flags, struct file **out)
{
    if (!path || !out)
        return ERR_INVALID_ARGUMENT;

    *out = NULL;
    struct vnode *vn = NULL;
    int result = vfs_lookup(path, &vn);
    if (result == ERR_NOT_FOUND && (flags & VFS_O_CREATE))
    {
        usize path_len = strlen(path);
        if (path_len && path[path_len - 1] == '/')
            return ERR_NOT_DIRECTORY;

        struct vnode *parent = NULL;
        char *leaf = NULL;
        result = vfs_resolve_parent_path(path, &parent, &leaf);
        if (result != ERR_SUCCESS)
            return result;

        if (!parent || !leaf || !parent->ops || !parent->ops->create)
        {
            free((void *)leaf);
            return ERR_NOT_SUPPORTED;
        }

        result = parent->ops->create(parent, leaf, VNODE_TYPE_FILE, &vn);
        free((void *)leaf);
        if (result != ERR_SUCCESS)
            return result;
    }
    else if (result != ERR_SUCCESS)
    {
        return result;
    }

    if (!vn)
        return ERR_INTERNAL;

    struct file *file = calloc(1, sizeof(*file));
    if (!file)
        return ERR_OUT_OF_MEMORY;

    vnode_ref(vn);
    file->vn = vn;
    file->flags = flags;
    file->ops = &vfs_generic_file_ops;
    file->refcount = 1;
    *out = file;
    return ERR_SUCCESS;
}

struct file *vfs_open_vnode(struct vnode *vn)
{
    if (!vn)
        return NULL;

    struct file *file = calloc(1, sizeof(*file));
    if (!file)
        return NULL;

    vnode_ref(vn);
    file->vn = vn;
    file->ops = &vfs_generic_file_ops;
    file->refcount = 1;
    return file;
}

int vfs_stat(const char *path, vfs_stat_t *stat)
{
    if (!path || !stat)
        return ERR_INVALID_ARGUMENT;

    struct vnode *vn = NULL;
    int result = vfs_lookup(path, &vn);
    if (result != ERR_SUCCESS)
        return result;
    if (!vn->ops || !vn->ops->getattr)
        return ERR_NOT_SUPPORTED;

    return vn->ops->getattr(vn, stat);
}

int vfs_mkdir(const char *path)
{
    if (!path)
        return ERR_INVALID_ARGUMENT;

    struct vnode *parent = NULL;
    char *leaf = NULL;
    int result = vfs_resolve_parent_path(path, &parent, &leaf);
    if (result != ERR_SUCCESS)
        return result;

    if (!parent || parent->type != VNODE_TYPE_DIR)
    {
        result = ERR_NOT_DIRECTORY;
    }
    else if (!parent->ops || !parent->ops->create)
    {
        result = ERR_NOT_SUPPORTED;
    }
    else
    {
        struct vnode *created = NULL;
        result = parent->ops->create(parent, leaf, VNODE_TYPE_DIR, &created);
        if (result == ERR_SUCCESS && !created)
            result = ERR_INTERNAL;
    }

    free((void *)leaf);
    return result;
}

int vfs_unlink(const char *path)
{
    if (!path)
        return ERR_INVALID_ARGUMENT;

    usize path_len = strlen(path);
    if (path_len > 1 && path[path_len - 1] == '/')
    {
        struct vnode *target = NULL;
        int lookup_result = vfs_lookup(path, &target);
        if (lookup_result != ERR_SUCCESS)
            return lookup_result;
        if (target->type != VNODE_TYPE_DIR)
            return ERR_NOT_DIRECTORY;
    }

    struct vnode *parent = NULL;
    char *leaf = NULL;
    int result = vfs_resolve_parent_path(path, &parent, &leaf);
    if (result != ERR_SUCCESS)
        return result;

    if (!parent || parent->type != VNODE_TYPE_DIR)
    {
        result = ERR_NOT_DIRECTORY;
    }
    else if (!parent->ops || !parent->ops->unlink)
    {
        result = ERR_NOT_SUPPORTED;
    }
    else
    {
        result = parent->ops->unlink(parent, leaf);
    }

    free((void *)leaf);
    return result;
}

int vfs_mount(const char *fs_name, const char *source, const char *target,
              const char *opts)
{
    if (!fs_name)
        return ERR_INVALID_ARGUMENT;

    struct fs_type *fst = find_fs_type(fs_name);
    if (!fst)
        return ERR_NOT_FOUND;

    return mount_do_mount(fst, source, target, opts);
}

int vfs_unmount(const char *target)
{
    return mount_do_unmount(target);
}

int vfs_create_exec(const char *path, exec_fn_t fn)
{
    if (!path || !fn)
        return ERR_INVALID_ARGUMENT;

    usize path_len = strlen(path);
    if (path_len && path[path_len - 1] == '/')
        return ERR_NOT_DIRECTORY;

    struct vnode *parent = NULL;
    char *leaf = NULL;
    int result = vfs_resolve_parent_path(path, &parent, &leaf);
    if (result != ERR_SUCCESS)
        return result;

    if (!parent || parent->type != VNODE_TYPE_DIR)
    {
        result = ERR_NOT_DIRECTORY;
    }
    else if (!parent->ops || !parent->ops->create)
    {
        result = ERR_NOT_SUPPORTED;
    }
    else
    {
        struct vnode *node = NULL;
        result = parent->ops->create(parent, leaf, VNODE_TYPE_EXEC, &node);
        if (result == ERR_SUCCESS)
        {
            if (!node)
                result = ERR_INTERNAL;
            else
                node->exec = fn;
        }
    }

    free((void *)leaf);
    return result;
}

int vfs_exec(const char *path)
{
    struct vnode *vn = NULL;
    int result = vfs_lookup(path, &vn);
    if (result != ERR_SUCCESS)
        return result;

    if (vn->type == VNODE_TYPE_EXEC && vn->exec)
    {
        vn->exec(NULL);
        return ERR_SUCCESS;
    }

    return exec_elf_vnode(vn);
}

int vfs_chroot(struct vnode *new_root)
{
    if (!new_root)
        return ERR_INVALID_ARGUMENT;

    return vfs_set_root(new_root);
}
