#include "fs/blockdev.h"
#include <stddef.h>
#include "libk/string.h"

static struct blockdev *g_blockdev_list = NULL;

int blockdev_register(struct blockdev *bd)
{
    if (!bd)
        return -1;

    if (!bd->name)
        return -1;

    /* Prevent duplicate registration */
    for (struct blockdev *it = g_blockdev_list; it; it = it->next)
    {
        if (strcmp(it->name, bd->name) == 0)
            return -1;
    }

    bd->next = g_blockdev_list;
    g_blockdev_list = bd;

    return 0;
}

int blockdev_unregister(struct blockdev *bd)
{
    if (!bd)
        return -1;

    struct blockdev **pp = &g_blockdev_list;

    while (*pp)
    {
        if (*pp == bd)
        {
            *pp = bd->next;
            bd->next = NULL;
            return 0;
        }

        pp = &(*pp)->next;
    }

    return -1;
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
        return -1;

    return bd->ops->read(bd, lba, count, buf);
}

int blockdev_write(struct blockdev *bd,
                   u32 lba,
                   u32 count,
                   const void *buf)
{
    if (!bd || !bd->ops || !bd->ops->write)
        return -1;

    return bd->ops->write(bd, lba, count, buf);
}