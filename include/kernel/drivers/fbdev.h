#ifndef KERNEL_DRIVERS_FBDEV_H
#define KERNEL_DRIVERS_FBDEV_H

#include "libc/types.h"
#include "kernel/fs/vnode.h"

/* Register the fb device hooks into a vnode (dev node) */
void fbdev_set_hooks(struct vnode *vn);

/* Helper to compute framebuffer size */
usize fbdev_framebuffer_size(void);

#endif
