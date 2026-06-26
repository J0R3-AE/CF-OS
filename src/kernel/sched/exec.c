#include "proc/proc.h"
#include "fs/vfs.h"
#include "elf/elf.h"
#include "mm/paging.h"
#include "mm/pmm.h"
#include "mm/heap.h"
#include "libk/mem.h"
#include "libk/math.h"
#include "libk/log.h"
#include "sched/sched.h"

extern void enter_user_mode(u32 eip, u32 esp);

#define USER_STACK_TOP 0xBFFFE000u
#define USER_STACK_SIZE (PAGE_SIZE * 16)

typedef struct user_exec_context
{
    u32 entry;
    u32 stack_top;
    u32 pd_phys;
} user_exec_context_t;

void klog_user_mode_entry(u32 eip, u32 esp)
{
    KLOG_INFO("enter_user_mode: eip=0x%x esp=0x%x", eip, esp);
}

static void user_thread_entry(void *arg)
{
    user_exec_context_t *ctx = (user_exec_context_t *)arg;
    if (!ctx)
        return;

    KLOG_INFO("user_thread_entry: entry=0x%x stack_top=0x%x pd_phys=0x%x", ctx->entry, ctx->stack_top, ctx->pd_phys);
    paging_switch_phys(ctx->pd_phys);
    enter_user_mode(ctx->entry, ctx->stack_top);

    for (;;)
        __asm__ volatile("cli; hlt");
}

int proc_exec_vnode(struct vnode *vn)
{
    return exec_elf_vnode(vn);
}

static int exec_elf_context(struct page_directory *pd, u32 pd_phys, Elf32_Addr entry)
{
    u32 stack_base = USER_STACK_TOP - USER_STACK_SIZE;
    for (u32 addr = stack_base; addr < USER_STACK_TOP; addr += PAGE_SIZE)
    {
        u32 phys = pmm_alloc_frame();
        if (!phys)
            return -1;

        paging_map_page(pd, addr, phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);

        void *kpage = paging_temp_map_frame(pd, phys);
        if (!kpage)
            return -1;

        memset(kpage, 0, PAGE_SIZE);
        paging_temp_unmap_frame(pd);
    }

    /* Create a small user-space trampoline page below the user stack.
     * Place it a few pages below the stack base to avoid accidental overlap
     * if the program grows/shrinks the stack. Reserve 4 pages of padding.
     */
    const u32 TRAMPOLINE_PADDING_PAGES = 4;
    u32 trampoline_addr = stack_base - (PAGE_SIZE * TRAMPOLINE_PADDING_PAGES);
    u32 tramp_phys = pmm_alloc_frame();
    if (!tramp_phys)
        return -1;

    paging_map_page(pd, trampoline_addr, tramp_phys, PAGE_PRESENT | PAGE_RW | PAGE_USER);
    void *tramp_kpage = paging_temp_map_frame(pd, tramp_phys);
    if (!tramp_kpage)
        return -1;

    /* Build trampoline code:
     * mov eax, <entry>
     * call eax
     * mov ebx, eax
     * mov eax, 1        ; SYS_exit
     * int 0x80
     */
    unsigned char tramp_code[] = {
        0xB8, 0x00, 0x00, 0x00, 0x00, /* mov eax, imm32 (entry) */
        0xFF, 0xD0,                 /* call eax */
        0x89, 0xC3,                 /* mov ebx, eax */
        0xB8, 0x01, 0x00, 0x00, 0x00, /* mov eax, 1 */
        0xCD, 0x80                  /* int 0x80 */
    };

    /* Patch in the entry address (little-endian) */
    u32 *entry_slot = (u32 *)&tramp_code[1];
    *entry_slot = (u32)entry;

    memcpy(tramp_kpage, tramp_code, sizeof(tramp_code));
    /* zero the rest of the page */
    if (sizeof(tramp_code) < PAGE_SIZE)
        memset((char *)tramp_kpage + sizeof(tramp_code), 0, PAGE_SIZE - sizeof(tramp_code));

    paging_temp_unmap_frame(pd);


    process_t *process = proc_create();
    if (!process)
        return -1;

    process->pd = pd;
    process->pd_phys = pd_phys;

    user_exec_context_t *ctx = malloc(sizeof(*ctx));
    if (!ctx)
        return -1;

    ctx->entry = entry;
    /* Use the trampoline as the initial eip so returns from user code
     * are handled by the trampoline which will exit the process. */
    ctx->entry = trampoline_addr;
    ctx->stack_top = USER_STACK_TOP;
    ctx->pd_phys = pd_phys;

    u8 *kernel_stack = malloc(4096);
    if (!kernel_stack)
    {
        free(ctx);
        return -1;
    }

    Thread *thread = thread_create(user_thread_entry, ctx, kernel_stack, 4096);
    if (!thread)
    {
        free(ctx);
        free(kernel_stack);
        return -1;
    }

    KLOG_INFO("exec_elf_context: created user thread %p for process %p entry=0x%x stack_top=0x%x",
              thread, process, entry, ctx->stack_top);

    proc_attach_thread(process, thread);
    sched_add(thread);

    return 0;
}

int exec_elf_vnode(struct vnode *vn)
{
    if (!vn)
        return -1;

    KLOG_INFO("exec_elf_vnode: vnode=%p type=%d ops=%p fs_data=%p", vn, vn->type, vn->ops, vn->fs_data);

    u32 pd_phys;
    struct page_directory *pd = paging_create_address_space(&pd_phys);
    if (!pd)
    {
        KLOG_ERROR("exec_elf_vnode: failed to create address space");
        return -1;
    }

    Elf32_Addr entry = 0;
    if (elf32_load_image(pd, vn, &entry) < 0)
    {
        KLOG_ERROR("exec_elf_vnode: elf32_load_image failed for vnode %p", vn);
        return -1;
    }

    KLOG_INFO("exec_elf_vnode: loaded entry=0x%x", entry);
    return exec_elf_context(pd, pd_phys, entry);
}

int exec_elf_image(const void *image, u32 size)
{
    KLOG_INFO("exec_elf_image: image=%p size=%u", image, size);
    if (!image || size == 0)
    {
        KLOG_ERROR("exec_elf_image: invalid image or size");
        return -1;
    }

    u32 pd_phys;
    struct page_directory *pd = paging_create_address_space(&pd_phys);
    if (!pd)
    {
        KLOG_ERROR("exec_elf_image: failed to create address space");
        return -1;
    }

    Elf32_Addr entry = 0;
    if (elf32_load_image_from_memory(pd, image, size, &entry) < 0)
    {
        KLOG_ERROR("exec_elf_image: failed to load ELF image from memory");
        return -1;
    }

    KLOG_INFO("exec_elf_image: loaded entry=0x%x pd_phys=0x%x", entry, pd_phys);
    return exec_elf_context(pd, pd_phys, entry);
}