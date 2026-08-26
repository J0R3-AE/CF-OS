#include "kernel/proc/exec.h"
#include "kernel/proc/process.h"

#include "kernel/fs/vfs.h"
#include "kernel/binfmt/elf.h"

#include "kernel/mm/paging.h"
#include "kernel/mm/pmm.h"
#include "kernel/mm/heap.h"

#include "kernel/sched/thread.h"
#include "kernel/sched/scheduler.h"

#include "libc/mem.h"
#include "libc/log.h"

extern void enter_user_mode(u32 eip, u32 esp);

#define USER_STACK_TOP  0xBFFFE000u
#define USER_STACK_SIZE (PAGE_SIZE * 16)

typedef struct
{
    u32 entry;
    u32 stack_top;
    u32 pd_phys;
} user_exec_context_t;


/* -------------------------------------------------------------------------- */
/* User thread                                                                */
/* -------------------------------------------------------------------------- */

static void user_thread_entry(void *arg)
{
    user_exec_context_t *ctx =
        (user_exec_context_t *)arg;

    if (!ctx)
        return;

    KLOG_INFO(
        "user thread: entry=0x%x stack=0x%x pd=0x%x",
        ctx->entry,
        ctx->stack_top,
        ctx->pd_phys);

    /*
     * Switch to this process's address space before entering ring 3.
     */
    paging_switch_phys(ctx->pd_phys);

    enter_user_mode(
        ctx->entry,
        ctx->stack_top);

    /*
     * enter_user_mode() should never return.
     */
    for (;;)
    {
        __asm__ volatile("cli");
        __asm__ volatile("hlt");
    }
}


/* -------------------------------------------------------------------------- */
/* Build user stack                                                           */
/* -------------------------------------------------------------------------- */

static int exec_create_user_stack(
    struct page_directory *pd)
{
    u32 stack_base =
        USER_STACK_TOP - USER_STACK_SIZE;

    for (u32 addr = stack_base;
         addr < USER_STACK_TOP;
         addr += PAGE_SIZE)
    {
        u32 phys = pmm_alloc_frame();

        if (!phys)
            return -1;

        paging_map_page(
    pd,
    addr,
    phys,
    PAGE_PRESENT |
    PAGE_RW |
    PAGE_USER);

        void *page =
            paging_temp_map_frame(pd, phys);

        if (!page)
            return -1;

        memset(page, 0, PAGE_SIZE);

        paging_temp_unmap_frame(pd);
    }

    return 0;
}


/* -------------------------------------------------------------------------- */
/* User trampoline                                                            */
/* -------------------------------------------------------------------------- */

static int exec_create_trampoline(
    struct page_directory *pd,
    Elf32_Addr entry,
    u32 stack_base,
    u32 *out_addr)
{
    const u32 padding_pages = 4;

    u32 trampoline_addr =
        stack_base - (PAGE_SIZE * padding_pages);

    u32 phys = pmm_alloc_frame();

    if (!phys)
        return -1;

    paging_map_page(
        pd,
        trampoline_addr,
        phys,
        PAGE_PRESENT |
        PAGE_RW |
        PAGE_USER);

    void *page =
        paging_temp_map_frame(pd, phys);

    if (!page)
        return -1;

    u8 code[] =
    {
        0xB8,
        0x00, 0x00, 0x00, 0x00,   /* mov eax, entry */
        0xFF, 0xD0,               /* call eax */
        0x89, 0xC3,               /* mov ebx, eax */
        0xB8,
        0x01, 0x00, 0x00, 0x00,  /* SYS_exit */
        0xCD, 0x80                /* int 0x80 */
    };

    *(u32 *)&code[1] = (u32)entry;

    memset(page, 0, PAGE_SIZE);
    memcpy(page, code, sizeof(code));

    paging_temp_unmap_frame(pd);

    *out_addr = trampoline_addr;

    return 0;
}


/* -------------------------------------------------------------------------- */
/* Create process/thread from loaded ELF                                      */
/* -------------------------------------------------------------------------- */

static int exec_create_process(
    struct page_directory *pd,
    u32 pd_phys,
    u32 entry)
{
    process_t *process =
        proc_create();

    if (!process)
    {
        KLOG_ERROR("exec: proc_create failed");
        return -1;
    }

    process->pd = pd;
    process->pd_phys = pd_phys;


    user_exec_context_t *ctx =
        malloc(sizeof(*ctx));

    if (!ctx)
        return -1;


    u32 stack_base =
        USER_STACK_TOP - USER_STACK_SIZE;

    u32 trampoline_addr = 0;

    if (exec_create_trampoline(
            pd,
            entry,
            stack_base,
            &trampoline_addr) < 0)
    {
        free(ctx);
        return -1;
    }


    ctx->entry = trampoline_addr;
    ctx->stack_top = USER_STACK_TOP;
    ctx->pd_phys = pd_phys;


    thread_t *thread =
        thread_create(
            user_thread_entry,
            ctx,
            4096);

    if (!thread)
    {
        free(ctx);
        return -1;
    }


    proc_attach_thread(
        process,
        thread);

    sched_add(thread);


    KLOG_INFO(
        "exec: pid=%d tid=%u entry=0x%x",
        process->pid,
        thread->tid,
        entry);

    return 0;
}


/* -------------------------------------------------------------------------- */
/* Execute ELF vnode                                                          */
/* -------------------------------------------------------------------------- */

int exec_elf_vnode(struct vnode *vn)
{
    if (!vn)
        return -1;

    KLOG_INFO(
        "exec_elf_vnode: vnode=%p",
        vn);


    u32 pd_phys = 0;

    struct page_directory *pd =
        paging_create_address_space(&pd_phys);

    if (!pd)
    {
        KLOG_ERROR(
            "exec: failed to create address space");

        return -1;
    }


    Elf32_Addr entry = 0;

    if (elf32_load_image(
            pd,
            vn,
            &entry) < 0)
    {
        KLOG_ERROR(
            "exec: ELF load failed");

        return -1;
    }


    if (exec_create_user_stack(pd) < 0)
    {
        KLOG_ERROR(
            "exec: user stack creation failed");

        return -1;
    }


    return exec_create_process(
        pd,
        pd_phys,
        entry);
}


/* -------------------------------------------------------------------------- */
/* Execute ELF from memory                                                    */
/* -------------------------------------------------------------------------- */

int exec_elf_image(
    const void *image,
    u32 size)
{
    if (!image || size == 0)
        return -1;


    u32 pd_phys = 0;

    struct page_directory *pd =
        paging_create_address_space(&pd_phys);

    if (!pd)
        return -1;


    Elf32_Addr entry = 0;

    if (elf32_load_image_from_memory(
            pd,
            image,
            size,
            &entry) < 0)
    {
        return -1;
    }


    if (exec_create_user_stack(pd) < 0)
        return -1;


    return exec_create_process(
        pd,
        pd_phys,
        entry);
}


/* -------------------------------------------------------------------------- */
/* Public process exec API                                                    */
/* -------------------------------------------------------------------------- */

int proc_exec_vnode(struct vnode *vn)
{
    return exec_elf_vnode(vn);
}

void klog_user_mode_entry(u32 eip, u32 esp)
{
    KLOG_INFO(
        "enter_user_mode: eip=0x%x esp=0x%x",
        eip,
        esp);
}