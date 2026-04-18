#include "fs/mount.h"

#include "libk/string.h"
#include "libk/types.h"
#include "libk/log.h"
#include "mm/heap.h"

/* Global intrusive list head for mounts */
static Link g_mounts;

static struct mount *find_mount_by_target(const char *target)
{
    struct mount *m;

    if (!target)
        return NULL;

    ListForEachEntry(m, g_mounts, link)
    {
        if (m->target && strcmp(m->target, target) == 0)
            return m;
    }

    return NULL;
}

int mount_init(void)
{
    LinkInit(&g_mounts);
    return EOK;
}

int mount_do_mount(struct fs_type *fst, const char *source, const char *target, const char *opts)
{
    if (!fst || !target)
        return EINVAL;

    if (find_mount_by_target(target))
        return ENOENT;

    struct mount *m = calloc(1, sizeof(*m));
    if (!m)
        return ENOMEM;

    m->type = fst;
    m->source = strdup(source ? source : "");
    m->target = strdup(target);

    if (!m->source || !m->target)
    {
        free(m->source);
        free(m->target);
        free(m);
        return ENOMEM;
    }

    m->fs_data = NULL;
    m->root_vnode = NULL;
    LinkInit(&m->link);

    if (fst->fs_ops && fst->fs_ops->mount)
    {
        int r = fst->fs_ops->mount(m, opts);
        if (r != 0)
        {
            free(m->source);
            free(m->target);
            free(m);
            return r;
        }
    }

    if (fst->root_vnode)
    {
        int r = fst->root_vnode(m, &m->root_vnode);
        if (r != 0)
        {
            if (fst->fs_ops && fst->fs_ops->unmount)
                fst->fs_ops->unmount(m);

            free(m->source);
            free(m->target);
            free(m);
            return r;
        }
    }

    ListBefore(&g_mounts, &m->link);

    if (strcmp(target, "/") == 0 && !vfs_get_root())
        vfs_set_root(m->root_vnode);

    return EOK;
}

int mount_do_unmount(const char *target)
{
    if (!target)
        return EINVAL;

    struct mount *m;

    ListForEachEntry(m, g_mounts, link)
    {
        if (m->target && strcmp(m->target, target) == 0)
        {
            if (m->type && m->type->fs_ops && m->type->fs_ops->unmount)
            {
                int r = m->type->fs_ops->unmount(m);
                if (r != 0)
                    return r;
            }

            if (vfs_get_root() == m->root_vnode)
                vfs_set_root(NULL);

            ListRemove(&m->link);

            free(m->source);
            free(m->target);
            free(m);
            return EOK;
        }
    }

    return ENOENT;
}

int mount_lookup(const char *target, struct mount **out_mount)
{
    if (!target || !out_mount)
        return EINVAL;

    struct mount *m = find_mount_by_target(target);
    if (!m)
        return ENOENT;

    *out_mount = m;
    return EOK;
}

int mount_lookup_vnode(const char *target, struct vnode **out_vnode)
{
    if (!target || !out_vnode)
        return EINVAL;

    struct mount *m;
    int r = mount_lookup(target, &m);
    if (r != 0)
        return r;

    if (!m->root_vnode)
        return ENOENT;

    *out_vnode = m->root_vnode;
    return EOK;
}

int mount_list(struct mount ***out_mounts, usize *out_count)
{
    if (!out_mounts || !out_count)
        return EINVAL;

    usize count = 0;
    struct mount *m;

    ListForEachEntry(m, g_mounts, link)
        count++;

    struct mount **arr = malloc(count * sizeof(*arr));
    if (!arr && count != 0)
        return ENOMEM;

    usize i = 0;
    ListForEachEntry(m, g_mounts, link)
        arr[i++] = m;

    *out_mounts = arr;
    *out_count = count;
    return EOK;
}

int mount_cleanup(void)
{
    while (!ListIsEmpty(&g_mounts))
    {
        struct mount *m = LinkData(g_mounts.next, struct mount, link);

        if (m->type && m->type->fs_ops && m->type->fs_ops->unmount)
            m->type->fs_ops->unmount(m);

        if (vfs_get_root() == m->root_vnode)
            vfs_set_root(NULL);

        ListRemove(&m->link);

        free(m->source);
        free(m->target);
        free(m);
    }

    return EOK;
}

int mount_shutdown(void)
{
    return mount_cleanup();
}

int mount_sync(void)
{
    struct mount *m;

    ListForEachEntry(m, g_mounts, link)
    {
        if (m->type && m->type->fs_ops && m->type->fs_ops->sync)
        {
            int r = m->type->fs_ops->sync(m);
            if (r != 0)
                return r;
        }
    }

    return EOK;
}

int mount_stat(const char *target, struct mount_stat *out_stat)
{
    if (!target || !out_stat)
        return EINVAL;

    struct mount *m = find_mount_by_target(target);
    if (!m)
        return ENOENT;

    out_stat->source = m->source;
    out_stat->target = m->target;
    out_stat->type = (m->type ? m->type->name : NULL);

    return EOK;
}