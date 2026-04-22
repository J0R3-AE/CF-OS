#include "proc/proc.h"
#include "sched/sched.h"

#include "elf/elf.h"
#include "fs/vfs.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "libk/mem.h"
#include "libk/log.h"
#include "libk/errno.h"

#include <stdint.h>

extern void enter_user_mode(uint32_t entry, uint32_t user_esp);

/* ---------------- USER STACK ---------------- */

#define USER_STACK_TOP 0xBFFFF000
#define USER_STACK_SIZE 0x1000

/* ---------------- CURRENT PROCESS ---------------- */

static process_t *get_proc(void)
{
    process_t *p = proc_current();
    if (p)
        return p;

    /* bootstrap process if scheduler not fully wired */
    static process_t boot;
    static int init = 0;

    if (!init)
    {
        memset(&boot, 0, sizeof(boot));
        boot.pid = 1;
        boot.ppid = 0;
        boot.alive = 1;
        init = 1;
    }

    return &boot;
}

/* ---------------- STACK SETUP ---------------- */

static int map_user_stack(struct page_directory *pd, uint32_t *esp_out)
{
    if (!pd || !esp_out)
        return -EINVAL;

    uint32_t phys = pmm_alloc_frame();
    if (!phys)
        return -ENOMEM;

    paging_map_page(pd,
                    USER_STACK_TOP - USER_STACK_SIZE,
                    phys,
                    PAGE_PRESENT | PAGE_RW | PAGE_USER);

    void *k = paging_temp_map_frame(pd, phys);
    if (!k)
        return -ENOMEM;

    memset(k, 0, PAGE_SIZE);
    paging_temp_unmap_frame(pd);

    *esp_out = USER_STACK_TOP;
    return 0;
}

/* ---------------- CORE EXEC ---------------- */

int proc_exec_vnode(struct vnode *vn)
{
    if (!vn)
        return -1;

    process_t *p = proc_current();
    if (!p)
        return -1;

    u32 pd_phys;
    struct page_directory *pd = paging_create_user_pd(&pd_phys);
    if (!pd)
        return -1;

    Elf32_Addr entry;
    int r = elf32_load_image(pd, vn, &entry);
    if (r < 0)
        return r;

    u32 user_esp = 0xBFFFF000;

    p->pd = pd;
    p->pd_phys = pd_phys;

    paging_switch_phys(pd_phys);

    klog_info("exec: entry=%x stack=%x", entry, user_esp);

    enter_user_mode((u32)entry, user_esp);

    return 0;
}
/* ---------------- COMPAT WRAPPER ---------------- */

int exec_elf_vnode(struct vnode *vn)
{
    process_t *p = get_proc();
    return proc_exec_vnode(vn);
}