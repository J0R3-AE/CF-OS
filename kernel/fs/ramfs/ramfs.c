#include "fs/ramfs.h"
#include "mm/heap.h"
#include "libk/string.h"
#include "libk/types.h"
#include "libk/mem.h"

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
    .root_vnode = ramfs_root_vnode};

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
    n->name = strdup(name ? name : "");
    n->type = type;
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
        return -1;

    rm->root = ramfs_new_node("", VNODE_TYPE_DIR);
    if (!rm->root)
    {
        free(rm);
        return -1;
    }

    mp->fs_data = rm;
    return 0;
}

static int ramfs_unmount_fn(struct mount *mp)
{
    /* TODO: recursively free tree; for now assume no leaks matter */
    (void)mp;
    return 0;
}

static int ramfs_sync_fn(struct mount *mp)
{
    (void)mp;
    return 0;
}

static int ramfs_root_vnode(struct mount *mp, struct vnode **out_root)
{
    struct ramfs_mount *rm = (struct ramfs_mount *)mp->fs_data;
    if (!rm || !rm->root)
        return -1;
    if (!rm->root_vnode_cache)
        rm->root_vnode_cache = ramfs_new_vnode(mp, rm->root);

    *out_root = rm->root_vnode_cache;
    return 0;
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
        return -1;

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;
    struct ramfs_node *c = ramfs_find_child(d, name);
    if (!c)
        return -1;

    *out = (struct vnode *)c->vnode_cache;
    if (*out)
        return 0;

    *out = ramfs_new_vnode(dir->mount, c);
    c->vnode_cache = *out;
    return (*out ? 0 : -1);
}

static int ramfs_create_vn(struct vnode *dir, const char *name, vnode_type_t type,
                           struct vnode **out)
{
    if (!dir || dir->type != VNODE_TYPE_DIR)
        return -1;

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;
    if (ramfs_find_child(d, name))
        return -1;

    struct ramfs_node *n = ramfs_new_node(name, type);
    if (!n)
        return -1;

    n->parent = d;
    n->sibling = d->children;
    d->children = n;

    *out = (struct vnode *)n->vnode_cache;
    if (*out)
        return 0;

    *out = ramfs_new_vnode(dir->mount, n);
    n->vnode_cache = *out;
    return (*out ? 0 : -1);
}

static int ramfs_unlink_vn(struct vnode *dir, const char *name)
{
    if (!dir || dir->type != VNODE_TYPE_DIR)
        return -1;

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;
    struct ramfs_node **pp = &d->children;
    while (*pp)
    {
        struct ramfs_node *c = *pp;
        if (strcmp(c->name, name) == 0)
        {
            *pp = c->sibling;
            /* TODO: free subtree */
            return 0;
        }
        pp = &c->sibling;
    }
    return -1;
}

static int ramfs_ensure_capacity(struct ramfs_node *n, usize new_cap)
{
    if (new_cap <= n->capacity)
        return 0;
    usize cap = n->capacity ? n->capacity : 64;
    while (cap < new_cap)
        cap *= 2;
    u8 *nd = realloc(n->data, cap);
    if (!nd)
        return -1;
    n->data = nd;
    n->capacity = cap;
    return 0;
}

static int ramfs_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out)
{
    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (n->type == VNODE_TYPE_DEV && n->dev_read)
    {
        usize got = n->dev_read(buf, len);
        if (out)
            *out = got;
        return 0;
    }

    if (off >= n->size)
    {
        if (out)
            *out = 0;
        return 0;
    }

    usize avail = n->size - off;
    if (len > avail)
        len = avail;

    memcpy(buf, n->data + off, len);
    if (out)
        *out = len;
    return 0;
}

static int ramfs_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out)
{
    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (n->type != VNODE_TYPE_FILE)
        return -1;

    usize end = off + len;
    if (end > n->capacity)
    {
        if (ramfs_ensure_capacity(n, end) != 0)
            return -1;
    }

    memcpy(n->data + off, buf, len);
    if (end > n->size)
        n->size = end;

    if (out)
        *out = len;
    return 0;
}

static int ramfs_readdir_vn(struct vnode *vn, usize index, const char **name_out,
                            vnode_type_t *type_out)
{
    struct ramfs_node *d = (struct ramfs_node *)vn->fs_data;
    if (d->type != VNODE_TYPE_DIR)
        return -1;

    usize i = 0;
    for (struct ramfs_node *c = d->children; c; c = c->sibling)
    {
        if (i == index)
        {
            if (name_out)
                *name_out = c->name;
            if (type_out)
                *type_out = c->type;
            return 0;
        }
        i++;
    }
    return -1; /* end */
}

static int ramfs_truncate_vn(struct vnode *vn, usize new_size)
{
    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (n->type != VNODE_TYPE_FILE)
        return -1;

    if (new_size > n->capacity)
    {
        if (ramfs_ensure_capacity(n, new_size) != 0)
            return -1;
    }
    n->size = new_size;
    return 0;
}

static int ramfs_getattr_vn(struct vnode *vn, void *stat_buf)
{
    if (!vn || !stat_buf)
        return -1;

    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (!n)
        return -1;

    vfs_stat_t *st = (vfs_stat_t *)stat_buf;

    st->type = n->type;
    st->size = n->size;
    st->capacity = n->capacity;

    // no real permissions yet
    st->mode = 0;
    st->uid = 0;
    st->gid = 0;

    // no time tracking yet
    st->atime = 0;
    st->mtime = 0;
    st->ctime = 0;

    return 0;
}
static int ramfs_setattr_vn(struct vnode *vn, const void *stat_buf)
{
    if (!vn || !stat_buf)
        return -1;

    struct ramfs_node *n = (struct ramfs_node *)vn->fs_data;
    if (!n)
        return -1;

    const vfs_stat_t *st = (const vfs_stat_t *)stat_buf;

    // Only support resizing files for now
    if (n->type == VNODE_TYPE_FILE)
    {
        if (st->size != n->size)
        {
            if (ramfs_truncate_vn(vn, st->size) != 0)
                return -1;
        }
    }

    // ignore permissions, times, etc for now

    return 0;
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
