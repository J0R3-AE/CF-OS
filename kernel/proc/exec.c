#include "proc/proc.h"
#include "fs/vfs.h"
#include "libk/errno.h"

int proc_exec_vnode(struct vnode *vn)
{
    (void)vn;
    return -1;
}

int exec_elf_vnode(struct vnode *vn)
{
    return proc_exec_vnode(vn);
}