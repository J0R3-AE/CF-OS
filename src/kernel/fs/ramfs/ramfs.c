#include "fs/ramfs.h"
#include "mm/heap.h"
#include "libk/string.h"
#include "libk/types.h"
#include "libk/mem.h"
#include "libk/log.h"

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
    KLOG_INFO("ramfs_new_node: allocating node for %s", name ? name : "<null>");
    struct ramfs_node *n = calloc(1, sizeof(*n));
    if (!n)
    {
        KLOG_ERROR("ramfs_new_node: calloc failed");
        return NULL;
    }
    KLOG_INFO("ramfs_new_node: calloc returned %p", n);
    n->name = strdup(name ? name : "");
    if (!n->name)
    {
        KLOG_ERROR("ramfs_new_node: strdup failed for %s", name ? name : "<null>");
        free(n);
        return NULL;
    }
    KLOG_INFO("ramfs_new_node: name duplicated %s", n->name);
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
    KLOG_INFO("ramfs_mount_fn: mounting ramfs for mount %p", mp);
    struct ramfs_mount *rm = calloc(1, sizeof(*rm));
    if (!rm)
    {
        KLOG_ERROR("ramfs_mount_fn: calloc failed for ramfs_mount");
        return -1;
    }

    rm->root = ramfs_new_node("", VNODE_TYPE_DIR);
    if (!rm->root)
    {
        KLOG_ERROR("ramfs_mount_fn: failed to create root node");
        free(rm);
        return -1;
    }

    mp->fs_data = rm;
    KLOG_INFO("ramfs_mount_fn: ramfs mounted with root node %p", rm->root);
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
    {
        KLOG_ERROR("ramfs_root_vnode: invalid ramfs mount state mp=%p rm=%p", mp, rm);
        return -1;
    }
    if (!rm->root_vnode_cache)
    {
        KLOG_INFO("ramfs_root_vnode: creating root vnode for mount %p", mp);
        rm->root_vnode_cache = ramfs_new_vnode(mp, rm->root);
        if (!rm->root_vnode_cache)
        {
            KLOG_ERROR("ramfs_root_vnode: failed to allocate root vnode");
            return -1;
        }
    }
    KLOG_INFO("ramfs_root_vnode: returning root vnode %p", rm->root_vnode_cache);
    *out_root = rm->root_vnode_cache;
    return 0;
}

/* Vnode ops */

static struct ramfs_node *ramfs_find_child(struct ramfs_node *dir, const char *name)
{
    for (struct ramfs_node *c = dir->children; c; c = c->sibling)
    {
        KLOG_INFO("ramfs_find_child: comparing child node=%p name=%s with target=%s", c, c->name, name);
        if (strcmp(c->name, name) == 0)
            return c;
    }
    return NULL;
}

static int ramfs_lookup_vn(struct vnode *dir, const char *name, struct vnode **out)
{
    if (!dir || dir->type != VNODE_TYPE_DIR)
    {
        KLOG_ERROR("ramfs_lookup_vn: invalid directory vnode %p for name '%s'", dir, name);
        return -1;
    }

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;
    struct ramfs_node *c = ramfs_find_child(d, name);
    if (!c)
    {
        KLOG_WARN("ramfs_lookup_vn: name '%s' not found in directory vnode %p", name, dir);
        return -1;
    }

    KLOG_INFO("ramfs_lookup_vn: found child node %p for name '%s'", c, name);
    *out = (struct vnode *)c->vnode_cache;
    if (*out)
    {
        KLOG_INFO("ramfs_lookup_vn: returning cached vnode %p for child %s", *out, name);
        return 0;
    }

    KLOG_INFO("ramfs_lookup_vn: creating vnode for child %s", name);
    *out = ramfs_new_vnode(dir->mount, c);
    c->vnode_cache = *out;
    if (!*out)
    {
        KLOG_ERROR("ramfs_lookup_vn: failed to allocate vnode for child %s", name);
        return -1;
    }
    return 0;
}

static int ramfs_create_vn(struct vnode *dir, const char *name, vnode_type_t type,
                           struct vnode **out)
{
    KLOG_INFO("ramfs_create_vn: creating %s in dir %p", name ? name : "<null>", dir);
    if (!dir || dir->type != VNODE_TYPE_DIR)
        return -1;

    struct ramfs_node *d = (struct ramfs_node *)dir->fs_data;
    if (ramfs_find_child(d, name))
    {
        KLOG_INFO("ramfs_create_vn: child %s already exists", name);
        return -1;
    }

    struct ramfs_node *n = ramfs_new_node(name, type);
    if (!n)
    {
        KLOG_ERROR("ramfs_create_vn: ramfs_new_node failed for %s", name);
        return -1;
    }

    KLOG_INFO("ramfs_create_vn: new node %p created", n);
    n->parent = d;
    n->sibling = d->children;
    d->children = n;

    *out = (struct vnode *)n->vnode_cache;
    if (*out)
    {
        KLOG_INFO("ramfs_create_vn: vnode cache hit for %s", name);
        return 0;
    }

    *out = ramfs_new_vnode(dir->mount, n);
    n->vnode_cache = *out;
    KLOG_INFO("ramfs_create_vn: vnode %p created for node %p", *out, n);
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
    KLOG_INFO("ramfs_read_vn: vnode=%p node=%p off=%u len=%u size=%u capacity=%u type=%d data=%p", vn, n, off, len, n->size, n->capacity, n->type, n->data);
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
    KLOG_INFO("ramfs_write_vn: vnode=%p node=%p off=%u len=%u size=%u capacity=%u type=%d data=%p", vn, n, off, len, n->size, n->capacity, n->type, n->data);
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

    KLOG_INFO("ramfs_write_vn: wrote %u bytes vnode=%p node=%p new_size=%u capacity=%u", len, vn, n, n->size, n->capacity);
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
