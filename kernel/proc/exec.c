#include "proc/proc.h"
#include "elf/elf.h"
#include "mm/paging.h"
#include "mm/heap.h"
#include "libk/log.h"
#include "libk/string.h"
#include "mm/pmm.h"

#define KERNEL_STACK_SIZE (PAGE_SIZE * 4)

int exec_elf_vnode(struct vnode *vn)
{
    Elf32_Addr entry;
    int r = elf32_load_image(kernel_pd, vn, &entry);
    if (r < 0)
        return r;

    klog_info("exec: jumping to kernel entry=%x", entry);

    /*
     * Kernel-mode execution path:
     * - no user page directory
     * - no ring3 switch
     * - no iret
     * - entry is called like a normal function
     *
     * This is only for kernel-side testing / helper binaries.
     */
    void (*entry_fn)(void) = (void (*)(void))(uintptr_t)entry;
    entry_fn();

    klog_err("exec: kernel entry returned unexpectedly");
    return -1;
}

int exec_path(const char *path)
{
    struct vnode *vn = vfs_resolve(path);
    if (!vn)
    {
        klog_err("exec_path: cannot resolve %s", path);
        return -1;
    }

    return exec_elf_vnode(vn);
}