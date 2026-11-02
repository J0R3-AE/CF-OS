#include "kernel/fs/vfs.h"
#include "libc/errno.h"
#include "kernel/mm/heap.h"

static int vfs_generic_read(struct file *f, void *buf, usize len, usize *out);
static int vfs_generic_write(struct file *f, const void *buf, usize len, usize *out);
static int vfs_generic_seek(struct file *f, usize off);

const struct file_ops vfs_generic_file_ops = {
    .read = vfs_generic_read,
    .write = vfs_generic_write,
    .seek = vfs_generic_seek,
};

int vfs_close(struct file *f)
{
    if (!f)
        return ERR_SUCCESS;

    if (f->refcount > 1)
    {
        --f->refcount;
        return ERR_SUCCESS;
    }

    if (f->vn)
        vnode_unref(f->vn);
    free(f);
    return ERR_SUCCESS;
}

static int vfs_generic_read(struct file *f, void *buf, usize len, usize *out)
{
    if (out)
        *out = 0;
    if (!f || !f->vn || (len && !buf))
        return ERR_INVALID_ARGUMENT;
    if (!f->vn->ops || !f->vn->ops->read)
        return ERR_NOT_SUPPORTED;

    usize done = 0;
    int result = f->vn->ops->read(f->vn, buf, f->offset, len, &done);
    if (done > len)
        return ERR_IO_ERROR;
    if (done > (usize)-1 - f->offset)
        return ERR_INVALID_ARGUMENT;
    if (result == ERR_SUCCESS)
        f->offset += done;
    if (out)
        *out = done;
    return result;
}

static int vfs_generic_write(struct file *f, const void *buf, usize len, usize *out)
{
    if (out)
        *out = 0;
    if (!f || !f->vn || (len && !buf))
        return ERR_INVALID_ARGUMENT;
    if (!f->vn->ops || !f->vn->ops->write)
        return ERR_NOT_SUPPORTED;

    usize done = 0;
    int result = f->vn->ops->write(f->vn, buf, f->offset, len, &done);
    if (done > len)
        return ERR_IO_ERROR;
    if (done > (usize)-1 - f->offset)
        return ERR_INVALID_ARGUMENT;
    if (result == ERR_SUCCESS)
        f->offset += done;
    if (out)
        *out = done;
    return result;
}

static int vfs_generic_seek(struct file *f, usize off)
{
    if (!f)
        return ERR_INVALID_ARGUMENT;

    f->offset = off;
    return ERR_SUCCESS;
}

int vfs_read(struct file *f, void *buf, usize len, usize *out)
{
    if (out)
        *out = 0;
    if (!f || !f->ops || !f->ops->read)
        return ERR_INVALID_ARGUMENT;

    return f->ops->read(f, buf, len, out);
}

int vfs_write(struct file *f, const void *buf, usize len, usize *out)
{
    if (out)
        *out = 0;
    if (!f || !f->ops || !f->ops->write)
        return ERR_INVALID_ARGUMENT;

    return f->ops->write(f, buf, len, out);
}

int vfs_seek(struct file *f, usize offset)
{
    if (!f || !f->ops || !f->ops->seek)
        return ERR_INVALID_ARGUMENT;

    return f->ops->seek(f, offset);
}

int vfs_readdir(struct vnode *vn, usize index, const char **name_out,
                vnode_type_t *type_out)
{
    if (!vn)
        return ERR_INVALID_ARGUMENT;
    if (!vn->ops || !vn->ops->readdir)
        return ERR_NOT_SUPPORTED;

    return vn->ops->readdir(vn, index, name_out, type_out);
}
