#include "kernel/fs/blockdev.h"
#include "libc/errno.h"
#include "libc/string.h"

static struct blockdev *g_blockdev_list = NULL;

int blockdev_register(struct blockdev *bd)
{
    if (!bd || !bd->name || !bd->name[0] || !bd->ops ||
        !bd->ops->read || !bd->ops->write ||
        !bd->sector_size || !bd->sector_count)
        return ERR_INVALID_ARGUMENT;

    /* Prevent duplicate registration */
    for (struct blockdev *it = g_blockdev_list; it; it = it->next)
    {
        if (strcmp(it->name, bd->name) == 0)
            return ERR_ALREADY_EXISTS;
    }

    bd->next = g_blockdev_list;
    g_blockdev_list = bd;

    return ERR_SUCCESS;
}

int blockdev_unregister(struct blockdev *bd)
{
    if (!bd)
        return ERR_INVALID_ARGUMENT;

    struct blockdev **pp = &g_blockdev_list;

    while (*pp)
    {
        if (*pp == bd)
        {
            *pp = bd->next;
            bd->next = NULL;
            return ERR_SUCCESS;
        }

        pp = &(*pp)->next;
    }

    return ERR_NOT_FOUND;
}

struct blockdev *blockdev_lookup(const char *name)
{
    if (!name)
        return NULL;

    for (struct blockdev *bd = g_blockdev_list; bd; bd = bd->next)
    {
        if (strcmp(bd->name, name) == 0)
            return bd;
    }

    return NULL;
}

int blockdev_read(struct blockdev *bd,
                  u32 lba,
                  u32 count,
                  void *buf)
{
    if (!bd || !bd->ops || !bd->ops->read)
        return ERR_INVALID_ARGUMENT;
    if (!count)
        return ERR_SUCCESS;
    if (!buf || lba >= bd->sector_count ||
        count > bd->sector_count - lba)
        return ERR_INVALID_ARGUMENT;

    return bd->ops->read(bd, lba, count, buf);
}

int blockdev_write(struct blockdev *bd,
                   u32 lba,
                   u32 count,
                   const void *buf)
{
    if (!bd || !bd->ops || !bd->ops->write)
        return ERR_INVALID_ARGUMENT;
    if (!count)
        return ERR_SUCCESS;
    if (!buf || lba >= bd->sector_count ||
        count > bd->sector_count - lba)
        return ERR_INVALID_ARGUMENT;

    return bd->ops->write(bd, lba, count, buf);
}