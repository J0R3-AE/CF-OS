#include "kernel/fs/vfs.h"

void file_ref(struct file *f)
{
    if (!f)
        return;
    ++f->refcount;
}

void file_unref(struct file *f)
{
    if (!f)
        return;

    if (f->refcount <= 1)
        vfs_close(f);
    else
        --f->refcount;
}