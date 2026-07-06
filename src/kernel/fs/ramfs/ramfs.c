#include "fs/ramfs.h"
#include "mm/heap.h"
#include "libk/string.h"
#include "libk/types.h"
#include "libk/mem.h"
#include "libk/errno.h"

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
static int ramfs_setattr_vn(struct vnode *vn, const void *stat_buf);

static const struct fs_ops ramfs_fs_ops = {
    .mount = ramfs_mount_fn,
    .unmount = ramfs_unmount_fn,
    .sync = ramfs_sync_fn,
};

static const struct vnode_ops ramfs_vnode_ops = {
    .lookup = ramfs_lookup_vn,
    .create = ramfs_create_vn,
    .unlink = ramfs_unlink_vn,
    .read = ramfs_read_vn,
    .write = ramfs_write_vn,
    .readdir = ramfs_readdir_vn,
    .truncate = ramfs_truncate_vn,
    .getattr = ramfs_getattr_vn,
    .setattr = ramfs_setattr_vn,
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

/* Helpers */

static struct ramfs_node *ramfs_new_node(const char *name, vnode_type_t type)
{
    struct ramfs_node *n = calloc(1, sizeof(*n));
    if (!n)
        return NULL;

    const char *src = name ? name : "";
    size_t len = strlen(src);

    n->name = malloc(len + 1);
    if (!n->name)
    {
        free(n);
        return NULL;
    }

    memcpy(n->name, src, len + 1);
    n->type = type;
    n->vnode_cache = NULL;
    n->children = NULL;
    n->parent = NULL;
    n->data = NULL;
    n->capacity = 0;
    n->size = 0;
    n->dev_read = NULL;
    n->dev_write = NULL;

    return n;
}

static struct vnode *ramfs_new_vnode(struct mount *mp, struct ramfs_node *rn)
{
    struct vnode *vn = calloc(1, sizeof(*vn));
    if (!vn)
        return NULL;

    vn->type = rn->type;
    vn->refcnt = 1;
    vn->mount = mp;
    vn->ops = &ramfs_vnode_ops;
    vn->fs_data = rn;

    return vn;
}

/* FS ops */

static int ramfs_mount_fn(struct mount *mp, const char *opts)
{
    (void)opts;

    struct ramfs_mount *rm = calloc(1, sizeof(*rm));
    if (!rm)
        return ERR_OUT_OF_MEMORY;

    rm->root = ramfs_new_node("", VNODE_TYPE_DIR);
    if (!rm->root)
    {
        free(rm);
        return ERR_OUT_OF_MEMORY;
    }

    rm->root_vnode_cache = NULL;
    mp->fs_data = rm;

    return ERR_SUCCESS;
}

static int ramfs_unmount_fn(struct mount *mp)
{
    (void)mp;
    /* TODO: free tree */
    return ERR_SUCCESS;
}

static int ramfs_sync_fn(struct mount *mp)
{
    (void)mp;
    return ERR_SUCCESS;
}

static int ramfs_root_vnode(struct mount *mp, struct vnode **out_root)
{
    struct ramfs_mount *rm = (struct ramfs_mount *)mp->fs_data;
    if (!rm || !rm->root)
        return ERR_INVALID_ARGUMENT;

    if (!rm->root_vnode_cache)
    {
        rm->root_vnode_cache = ramfs_new_vnode(mp, rm->root);
        if (!rm->root_vnode_cache)
            return ERR_OUT_OF_MEMORY;
    }

    *out_root = rm->root_vnode_cache;
    return ERR_SUCCESS;
}

/* Vnode ops */

static struct ramfs_node *ramfs_find_child(struct ramfs_node *dir, const char *name)
{
    for (struct ramfs_node *c = dir->children; c; c = c->sibling)
    {
        if (strcmp(c->name, name) == 0)
            return c;
    }
    return NULL;
}

static int ramfs_lookup_vn(struct vnode *dir, const char *name, struct vnode **out)
{
    if (!dir || dir->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;
    struct ramfs_node *c = ramfs_find_child(d, name);
    if (!c)
        return ERR_NOT_FOUND;

    if (c->vnode_cache)
    {
        *out = c->vnode_cache;
        return ERR_SUCCESS;
    }

    *out = ramfs_new_vnode(dir->mount, c);
    if (!*out)
        return ERR_OUT_OF_MEMORY;

    c->vnode_cache = *out;
    return ERR_SUCCESS;
}

static int ramfs_create_vn(struct vnode *dir, const char *name, vnode_type_t type,
                           struct vnode **out)
{
    if (!dir || dir->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;

    if (ramfs_find_child(d, name))
        return ERR_ALREADY_EXISTS;

    struct ramfs_node *n = ramfs_new_node(name, type);
    if (!n)
        return ERR_OUT_OF_MEMORY;

    n->parent = d;
    n->sibling = d->children;
    d->children = n;

    if (n->vnode_cache)
    {
        *out = n->vnode_cache;
        return ERR_SUCCESS;
    }

    *out = ramfs_new_vnode(dir->mount, n);
    if (!*out)
        return ERR_OUT_OF_MEMORY;

    n->vnode_cache = *out;
    return ERR_SUCCESS;
}

static int ramfs_unlink_vn(struct vnode *dir, const char *name)
{
    if (!dir || dir->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;
    struct ramfs_node **pp = &d->children;

    while (*pp)
    {
        struct ramfs_node *c = *pp;
        if (strcmp(c->name, name) == 0)
        {
            *pp = c->sibling;
            /* TODO: free subtree */
            return ERR_SUCCESS;
        }
        pp = &c->sibling;
    }

    return ERR_NOT_FOUND;
}

static int ramfs_ensure_capacity(struct ramfs_node *n, usize new_cap)
{
    if (new_cap <= n->capacity)
        return ERR_SUCCESS;

    usize cap = n->capacity ? n->capacity : 64;
    while (cap < new_cap)
        cap *= 2;

    u8 *nd = realloc(n->data, cap);
    if (!nd)
        return ERR_OUT_OF_MEMORY;

    n->data = nd;
    n->capacity = cap;
    return ERR_SUCCESS;
}

static int ramfs_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out)
{
    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;

    if (n->type == VNODE_TYPE_DEV && n->dev_read)
    {
        usize got = n->dev_read(buf, len);
        if (out)
            *out = got;
        return ERR_SUCCESS;
    }

    if (off >= n->size)
    {
        if (out)
            *out = 0;
        return ERR_END_OF_FILE;
    }

    usize avail = n->size - off;
    if (len > avail)
        len = avail;

    memcpy(buf, n->data + off, len);
    if (out)
        *out = len;

    return ERR_SUCCESS;
}

static int ramfs_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out)
{
    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;

    if (n->type != VNODE_TYPE_FILE)
        return ERR_IS_DIRECTORY;

    usize end = off + len;

    if (end > n->capacity)
    {
        int r = ramfs_ensure_capacity(n, end);
        if (r < 0)
            return r;
    }

    memcpy(n->data + off, buf, len);
    if (end > n->size)
        n->size = end;

    if (out)
        *out = len;

    return ERR_SUCCESS;
}

static int ramfs_readdir_vn(struct vnode *vn, usize index, const char **name_out,
                            vnode_type_t *type_out)
{
    struct ramfs_node *d = (struct ramfs_node *)vn->fs_data;

    if (d->type != VNODE_TYPE_DIR)
        return ERR_NOT_DIRECTORY;

    usize i = 0;
    for (struct ramfs_node *c = d->children; c; c = c->sibling)
    {
        if (i == index)
        {
            if (name_out)
                *name_out = c->name;
            if (type_out)
                *type_out = c->type;
            return ERR_SUCCESS;
        }
        i++;
    }

    return ERR_NOT_FOUND; /* end of directory */
}

static int ramfs_truncate_vn(struct vnode *vn, usize new_size)
{
    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;

    if (n->type != VNODE_TYPE_FILE)
        return ERR_IS_DIRECTORY;

    if (new_size > n->capacity)
    {
        int r = ramfs_ensure_capacity(n, new_size);
        if (r < 0)
            return r;
    }

    n->size = new_size;
    return ERR_SUCCESS;
}

static int ramfs_getattr_vn(struct vnode *vn, void *stat_buf)
{
    if (!vn || !stat_buf)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (!n)
        return ERR_INVALID_ARGUMENT;

    vfs_stat_t *st = (vfs_stat_t *)stat_buf;

    st->type = n->type;
    st->size = n->size;
    st->capacity = n->capacity;

    st->mode = 0;
    st->uid = 0;
    st->gid = 0;

    st->atime = 0;
    st->mtime = 0;
    st->ctime = 0;

    return ERR_SUCCESS;
}

static int ramfs_setattr_vn(struct vnode *vn, const void *stat_buf)
{
    if (!vn || !stat_buf)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (!n)
        return ERR_INVALID_ARGUMENT;

    const vfs_stat_t *st = (const vfs_stat_t *)stat_buf;

    if (n->type == VNODE_TYPE_FILE)
    {
        if (st->size != n->size)
        {
            int r = ramfs_truncate_vn(vn, st->size);
            if (r < 0)
                return r;
        }
    }

    return ERR_SUCCESS;
}

void ramfs_set_dev_hooks(struct vnode *vn,
                         int (*dev_read)(void *buf, usize len),
                         int (*dev_write)(const void *buf, usize len))
{
    if (!vn || vn->type != VNODE_TYPE_DEV)
        return;

    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (!n)
        return;

    n->dev_read = dev_read;
    n->dev_write = dev_write;
}
