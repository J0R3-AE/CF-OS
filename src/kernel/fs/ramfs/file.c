#include "ramfs_internal.h"
#include "libc/errno.h"
#include "libc/mem.h"
#include "libc/types.h"
#include "kernel/mm/heap.h"

static int ramfs_ensure_capacity(struct ramfs_node *node, usize new_capacity)
{
    if (new_capacity <= node->capacity)
        return ERR_SUCCESS;

    usize capacity = node->capacity ? node->capacity : 64;
    while (capacity < new_capacity)
    {
        if (capacity > (usize)-1 / 2)
        {
            capacity = new_capacity;
            break;
        }
        capacity *= 2;
    }

    u8 *data = realloc(node->data, capacity);
    if (!data)
        return ERR_OUT_OF_MEMORY;

    node->data = data;
    node->capacity = capacity;
    return ERR_SUCCESS;
}

int ramfs_read_vn(struct vnode *vn, void *buf, usize off, usize len, usize *out)
{
    if (out)
        *out = 0;
    if (!vn || (len && !buf))
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node *node = vn->fs_data;
    if (!node)
        return ERR_INVALID_ARGUMENT;

    if (node->type == VNODE_TYPE_DEV)
    {
        if (!node->dev_read)
            return ERR_NOT_SUPPORTED;

        int read = node->dev_read(buf, len);
        if (read < 0)
            return read;
        if ((usize)read > len)
            return ERR_IO_ERROR;
        if (out)
            *out = (usize)read;
        return ERR_SUCCESS;
    }

    if (node->type == VNODE_TYPE_DIR)
        return ERR_IS_DIRECTORY;
    if (node->type != VNODE_TYPE_FILE && node->type != VNODE_TYPE_EXEC)
        return ERR_NOT_SUPPORTED;
    if (off >= node->size || len == 0)
        return ERR_SUCCESS;

    usize available = node->size - off;
    if (len > available)
        len = available;

    memcpy(buf, node->data + off, len);
    if (out)
        *out = len;
    return ERR_SUCCESS;
}

int ramfs_write_vn(struct vnode *vn, const void *buf, usize off, usize len, usize *out)
{
    if (out)
        *out = 0;
    if (!vn || (len && !buf))
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node *node = vn->fs_data;
    if (!node)
        return ERR_INVALID_ARGUMENT;

    if (node->type == VNODE_TYPE_DEV)
    {
        if (!node->dev_write)
            return ERR_NOT_SUPPORTED;

        int written = node->dev_write(buf, len);
        if (written < 0)
            return written;
        if ((usize)written > len)
            return ERR_IO_ERROR;
        if (out)
            *out = (usize)written;
        return ERR_SUCCESS;
    }

    if (node->type == VNODE_TYPE_DIR)
        return ERR_IS_DIRECTORY;
    if (node->type != VNODE_TYPE_FILE && node->type != VNODE_TYPE_EXEC)
        return ERR_NOT_SUPPORTED;
    if (len == 0)
        return ERR_SUCCESS;
    if (len > (usize)-1 - off)
        return ERR_INVALID_ARGUMENT;

    usize end = off + len;
    int result = ramfs_ensure_capacity(node, end);
    if (result != ERR_SUCCESS)
        return result;

    if (off > node->size)
        memset(node->data + node->size, 0, off - node->size);
    memcpy(node->data + off, buf, len);
    if (end > node->size)
        node->size = end;

    if (out)
        *out = len;
    return ERR_SUCCESS;
}

int ramfs_truncate_vn(struct vnode *vn, usize new_size)
{
    if (!vn)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node *node = vn->fs_data;
    if (!node)
        return ERR_INVALID_ARGUMENT;
    if (node->type == VNODE_TYPE_DIR)
        return ERR_IS_DIRECTORY;
    if (node->type != VNODE_TYPE_FILE && node->type != VNODE_TYPE_EXEC)
        return ERR_NOT_SUPPORTED;

    int result = ramfs_ensure_capacity(node, new_size);
    if (result != ERR_SUCCESS)
        return result;

    if (new_size > node->size)
        memset(node->data + node->size, 0, new_size - node->size);
    node->size = new_size;
    return ERR_SUCCESS;
}

int ramfs_getattr_vn(struct vnode *vn, void *stat_buf)
{
    if (!vn || !stat_buf)
        return ERR_INVALID_ARGUMENT;

    struct ramfs_node *node = vn->fs_data;
    if (!node)
        return ERR_INVALID_ARGUMENT;

    vfs_stat_t *stat = stat_buf;
    stat->type = node->type;
    stat->size = node->size;
    stat->capacity = node->capacity;
    stat->mode = 0;
    stat->uid = 0;
    stat->gid = 0;
    stat->atime = 0;
    stat->mtime = 0;
    stat->ctime = 0;
    return ERR_SUCCESS;
}

int ramfs_setattr_vn(struct vnode *vn, const void *stat_buf)
{
    if (!vn || !stat_buf)
        return ERR_INVALID_ARGUMENT;

    const vfs_stat_t *stat = stat_buf;
    return ramfs_truncate_vn(vn, stat->size);
}

void ramfs_set_dev_hooks(struct vnode *vn, int (*dev_read)(void *buf, usize len),
                         int (*dev_write)(const void *buf, usize len))
{
    if (!vn || vn->type != VNODE_TYPE_DEV)
        return;

    struct ramfs_node *node = vn->fs_data;
    if (!node)
        return;

    node->dev_read = dev_read;
    node->dev_write = dev_write;
}
