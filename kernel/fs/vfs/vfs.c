#include "fs/vfs.h"
#include "fs/fs_types.h"
#include "fs/mount.h"
#include "libk/string.h"
#include "libk/log.h"
#include "mm/heap.h"
#include "proc/proc.h"
#include "libk/link.h"

/* -------------------------------------------------------------------------- */
/* Globals                                                                    */
/* -------------------------------------------------------------------------- */

Link g_fs_types; /* intrusive FS registry list head */
static struct vnode *g_root_vnode;

/* -------------------------------------------------------------------------- */
/* File ops (unchanged)                                                      */
/* -------------------------------------------------------------------------- */

static int vfs_generic_read(struct file *f, void *buf, usize len, usize *out);
static int vfs_generic_write(struct file *f, const void *buf, usize len, usize *out);
static int vfs_generic_seek(struct file *f, usize off);
static int vfs_generic_close(struct file *f);

const struct file_ops vfs_generic_file_ops = {
    .read = vfs_generic_read,
    .write = vfs_generic_write,
    .seek = vfs_generic_seek,
    .close = vfs_generic_close,
};

/* -------------------------------------------------------------------------- */
/* FS REGISTRY (NOW LINK-BASED)                                               */
/* -------------------------------------------------------------------------- */

int vfs_register_fs(struct fs_type *fst)
{
    if (!fst || !fst->name)
        return EINVAL;

    LinkInit(&fst->link);
    LinkBefore(&g_fs_types, &fst->link);

    return EOK;
}

static struct fs_type *find_fs_type(const char *name)
{
    struct fs_type *it;

    ListForEachEntry(it, g_fs_types, link)
    {
        if (strcmp(it->name, name) == 0)
            return it;
    }

    return NULL;
}

/* -------------------------------------------------------------------------- */
/* ROOT VNODE                                                                 */
/* -------------------------------------------------------------------------- */

int vfs_set_root(struct vnode *root)
{
    g_root_vnode = root;
    return EOK;
}

struct vnode *vfs_get_root(void)
{
    return g_root_vnode;
}

/* -------------------------------------------------------------------------- */
/* LOOKUP                                                                     */
/* -------------------------------------------------------------------------- */

int vfs_lookup(const char *path, struct vnode **out)
{
    if (!path || !out)
        return EINVAL;

    klog_info("vfs_lookup: %s", path);

    if (path[0] != '/')
        return ENOENT;

    struct vnode *cur = g_root_vnode;
    if (!cur)
        return ENOENT;

    const char *p = path + 1;
    char name[64];

    while (*p)
    {
        usize len = 0;

        while (p[len] && p[len] != '/' && len < sizeof(name) - 1)
        {
            name[len] = p[len];
            len++;
        }

        name[len] = '\0';

        if (len == 0)
        {
            p++;
            continue;
        }

        if (!cur->ops || !cur->ops->lookup)
            return ENOENT;

        struct vnode *next = NULL;
        int r = cur->ops->lookup(cur, name, &next);
        if (r != 0)
            return r;

        cur = next;

        if (p[len] == '/')
            p += len + 1;
        else
            p += len;
    }

    *out = cur;
    return EOK;
}

struct vnode *vfs_resolve(const char *path)
{
    struct vnode *vn;
    if (vfs_lookup(path, &vn) == 0)
        return vn;
    return NULL;
}

/* -------------------------------------------------------------------------- */
/* PATH RESOLVE                                                               */
/* -------------------------------------------------------------------------- */

struct vnode *vfs_resolve_path(const char *path,
                               struct vnode **parent_out,
                               const char **leaf_name_out)
{
    if (!path || !parent_out || !leaf_name_out)
        return NULL;

    if (path[0] != '/')
        return NULL;

    struct vnode *cur = g_root_vnode;
    if (!cur)
        return NULL;

    const char *p = path + 1;

    while (*p)
    {
        char name[64];
        usize len = 0;

        while (p[len] && p[len] != '/' && len < sizeof(name) - 1)
        {
            name[len] = p[len];
            len++;
        }

        name[len] = '\0';

        if (len == 0)
        {
            p++;
            continue;
        }

        bool has_more = (p[len] == '/');

        if (!has_more)
        {
            char *leaf = strdup(name);
            if (!leaf)
                return NULL;

            *parent_out = cur;
            *leaf_name_out = leaf;

            return cur;
        }

        if (!cur->ops || !cur->ops->lookup)
            return NULL;

        struct vnode *next = NULL;
        int r = cur->ops->lookup(cur, name, &next);
        if (r != 0)
            return NULL;

        cur = next;
        p += len + 1;
    }

    return NULL;
}

/* -------------------------------------------------------------------------- */
/* OPEN / CLOSE                                                              */
/* -------------------------------------------------------------------------- */

int vfs_open(const char *path, u32 flags, struct file **out)
{
    if (!path || !out)
        return EINVAL;

    klog_info("vfs_open: %s flags=%x", path, flags);

    struct vnode *vn = NULL;
    int r = vfs_lookup(path, &vn);

    if (r != 0 && (flags & VFS_O_CREATE))
    {
        struct vnode *parent = NULL;
        const char *leaf = NULL;

        if (!vfs_resolve_path(path, &parent, &leaf))
            return ENOENT;

        if (!parent || !leaf || leaf[0] == '\0')
        {
            free((void *)leaf);
            return ENOENT;
        }

        if (!parent->ops || !parent->ops->create)
        {
            free((void *)leaf);
            return ENOENT;
        }

        r = parent->ops->create(parent, leaf, VNODE_TYPE_FILE, &vn);
        free((void *)leaf);

        if (r != 0)
            return r;
    }
    else if (r != 0)
    {
        return r;
    }

    struct file *f = calloc(1, sizeof(*f));
    if (!f)
        return ENOMEM;

    f->vn = vn;
    f->offset = 0;
    f->flags = flags;
    f->ops = &vfs_generic_file_ops;

    *out = f;
    return EOK;
}

struct file *vfs_open_vnode(struct vnode *vn)
{
    if (!vn)
        return NULL;

    struct file *f = calloc(1, sizeof(*f));
    if (!f)
        return NULL;

    f->vn = vn;
    f->offset = 0;
    f->flags = 0;
    f->ops = &vfs_generic_file_ops;

    return f;
}

int vfs_close(struct file *f)
{
    if (!f)
        return EOK;

    free(f);
    return EOK;
}

/* -------------------------------------------------------------------------- */
/* FILE OPS                                                                  */
/* -------------------------------------------------------------------------- */

static int vfs_generic_read(struct file *f, void *buf, usize len, usize *out)
{
    if (!f || !f->vn || !f->vn->ops || !f->vn->ops->read)
        return EINVAL;

    usize done = 0;
    int r = f->vn->ops->read(f->vn, buf, f->offset, len, &done);

    if (r == 0)
        f->offset += done;

    if (out)
        *out = done;

    return r;
}

static int vfs_generic_write(struct file *f, const void *buf, usize len, usize *out)
{
    if (!f || !f->vn || !f->vn->ops || !f->vn->ops->write)
        return EINVAL;

    usize done = 0;
    int r = f->vn->ops->write(f->vn, buf, f->offset, len, &done);

    if (r == 0)
        f->offset += done;

    if (out)
        *out = done;

    return r;
}

static int vfs_generic_seek(struct file *f, usize off)
{
    if (!f)
        return EINVAL;

    f->offset = off;
    return EOK;
}

static int vfs_generic_close(struct file *f)
{
    return vfs_close(f);
}

/* -------------------------------------------------------------------------- */
/* PUBLIC READ/WRITE                                                         */
/* -------------------------------------------------------------------------- */

int vfs_read(struct file *f, void *buf, usize len, usize *out)
{
    if (!f || !f->ops || !f->ops->read)
        return EINVAL;

    return f->ops->read(f, buf, len, out);
}

int vfs_write(struct file *f, const void *buf, usize len, usize *out)
{
    if (!f || !f->ops || !f->ops->write)
        return EINVAL;

    return f->ops->write(f, buf, len, out);
}

/* -------------------------------------------------------------------------- */
/* MOUNT                                                                     */
/* -------------------------------------------------------------------------- */

int vfs_mount(const char *fs_name, const char *source, const char *target, const char *opts)
{
    struct fs_type *fst = find_fs_type(fs_name);
    if (!fst)
        return ENOENT;

    return mount_do_mount(fst, source, target, opts);
}

int vfs_unmount(const char *target)
{
    return mount_do_unmount(target);
}

/* -------------------------------------------------------------------------- */
/* EXEC                                                                      */
/* -------------------------------------------------------------------------- */

int vfs_create_exec(const char *path, exec_fn_t fn)
{
    if (!path)
        return EINVAL;

    klog_info("vfs_create_exec: %s", path);

    struct vnode *parent = NULL;
    const char *name = NULL;

    if (!vfs_resolve_path(path, &parent, &name))
        return ENOENT;

    if (!parent || !name || name[0] == '\0')
    {
        free((void *)name);
        return ENOENT;
    }

    if (!parent->ops || !parent->ops->create)
    {
        free((void *)name);
        return ENOENT;
    }

    struct vnode *node = NULL;
    int r = parent->ops->create(parent, name, VNODE_TYPE_EXEC, &node);

    free((void *)name);

    if (r != 0)
        return r;

    node->exec = fn;

    return EOK;
}

int vfs_exec(const char *path)
{
    struct vnode *vn;
    int r = vfs_lookup(path, &vn);
    if (r != 0)
        return r;

    /* function-backed exec */
    if (vn->type == VNODE_TYPE_EXEC && vn->exec)
    {
        vn->exec(NULL);
        return 0;
    }

    /* file-backed exec (ELF) */
    return exec_elf_vnode(vn);
}