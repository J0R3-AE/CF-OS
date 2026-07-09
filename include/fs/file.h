#pragma once

#include "libk/types.h"

struct vnode;
struct file;

/* File Operation          */
struct file_ops
{
    int (*read)(struct file *f, void *buf, usize len, usize *out);
    int (*write)(struct file *f,const void *buf, usize len, usize *out);
    int (*seek)(struct file *f,usize off);
    int (*close)(struct file *f);
};

/* Open File */
struct file
{
    struct vnode *vn;
    usize offset;
    u32 flags;
    const struct file_ops *ops;
    void *priv;
    int refcount;
};