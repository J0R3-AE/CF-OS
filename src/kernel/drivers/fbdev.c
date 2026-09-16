#include "kernel/drivers/fbdev.h"
#include "kernel/drivers/framebuffer.h"
#include "kernel/fs/ramfs.h"
#include "libc/types.h"
#include "libc/errno.h"
#include "libc/mem.h"

static int fbdev_read(void *buf, usize len)
{
    if (!buf || len == 0)
        return ERR_INVALID_ARGUMENT;

    usize full = fbdev_framebuffer_size();
    if (len > full)
        len = full;

    memcpy(buf, (void *)framebuffer_address(), len);
    return (int)len;
}

static int fbdev_write(const void *buf, usize len)
{
    if (!buf || len == 0)
        return ERR_INVALID_ARGUMENT;

    usize full = fbdev_framebuffer_size();

    /* Require full-buffer writes for simplicity */
    if (len != full)
        return ERR_INVALID_ARGUMENT;

    memcpy((void *)framebuffer_address(), buf, len);
    return (int)len;
}

void fbdev_set_hooks(struct vnode *vn)
{
    if (!vn || vn->type != VNODE_TYPE_DEV)
        return;

    ramfs_set_dev_hooks(vn, fbdev_read, fbdev_write);
}

usize fbdev_framebuffer_size(void)
{
    u32 pitch = framebuffer_pitch();
    u32 h = framebuffer_height();
    return (usize)pitch * (usize)h;
}
