#include "kernel/proc/proc.h"
#include "kernel/fs/vfs.h"
#include "kernel/binfmt/elf.h"
#include "kernel/mm/paging.h"
#include "kernel/mm/pmm.h"
#include "kernel/mm/heap.h"
#include "libc/mem.h"
#include "libc/math.h"
#include "libc/string.h"
#include "kernel/log.h"
#include "kernel/sched/sched.h"

extern void enter_user_mode(u32 eip, u32 esp);

#define USER_STACK_TOP 0xBFFFE000u
#define USER_STACK_SIZE (PAGE_SIZE * 16)

typedef struct user_exec_context
{
    u32 entry;
    u32 stack_top;
    u32 argc;
    u32 argv;
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

    KLOG_INFO("user_thread_entry: entry=0x%x stack_top=0x%x", ctx->entry, ctx->stack_top);
    enter_user_mode(ctx->entry, ctx->stack_top);

    for (;;)
        __asm__ volatile("cli; hlt");
}

int proc_exec_vnode(struct vnode *vn)
{
    return exec_elf_vnode(vn);
}

static int exec_copy_to_user_space(struct page_directory *pd, u32 address,
                                   const void *source, u32 length)
{
    const u8 *bytes = (const u8 *)source;
    while (length)
    {
        u32 page = address & ~(PAGE_SIZE - 1);
        u32 pde = pd->entries[(page >> 22) & 0x3FF];
        if (!(pde & PAGE_PRESENT))
            return -1;
        u32 *pt = (u32 *)(pde & ~0xFFFu);
        u32 pte = pt[(page >> 12) & 0x3FF];
        if (!(pte & PAGE_PRESENT) || !(pte & PAGE_USER))
            return -1;

        u32 offset = address & (PAGE_SIZE - 1);
        u32 amount = PAGE_SIZE - offset;
        if (amount > length)
            amount = length;
        void *mapped = paging_temp_map_frame(pd, pte & ~0xFFFu);
        if (!mapped)
            return -1;
        memcpy((u8 *)mapped + offset, bytes, amount);
        paging_temp_unmap_frame(pd);
        address += amount;
        bytes += amount;
        length -= amount;
    }
    return 0;
}

static int exec_build_stack(struct page_directory *pd, int argc,
                            const char *const argv[], u32 *stack_out,
                            u32 *argv_out)
{
    if (argc < 0 || argc > 32 || (argc && !argv))
        return -1;

    u32 user_argv[33] = {0};
    u32 stack = USER_STACK_TOP;
    for (int i = argc - 1; i >= 0; i--)
    {
        usize length = strlen(argv[i]) + 1;
        if (length > PAGE_SIZE * 4 || length > stack - (USER_STACK_TOP - USER_STACK_SIZE))
            return -1;
        stack -= (u32)length;
        if (exec_copy_to_user_space(pd, stack, argv[i], (u32)length) != 0)
            return -1;
        user_argv[i] = stack;
    }

    stack &= ~0x3u;
    u32 vector_size = ((u32)argc + 1) * sizeof(u32);
    if (vector_size > stack - (USER_STACK_TOP - USER_STACK_SIZE))
        return -1;
    stack -= vector_size;
    if (exec_copy_to_user_space(pd, stack, user_argv, vector_size) != 0)
        return -1;

    *stack_out = stack & ~0xFu;
    *argv_out = stack;
    return 0;
}

static int exec_elf_context(struct page_directory *pd, u32 pd_phys,
                            Elf32_Addr entry, int argc,
                            const char *const argv[])
{
    u32 stack_base = USER_STACK_TOP - USER_STACK_SIZE;
    for (u32 addr = stack_base; addr < USER_STACK_TOP; addr += PAGE_SIZE)
    {
        u32 phys = pmm_alloc_frame();
        if (!phys)
        {
            paging_destroy_address_space(pd, pd_phys);
            return -1;
            }

        if (paging_map_page(pd, addr, phys,
                            PAGE_PRESENT | PAGE_RW | PAGE_USER) != 0)
        {
            pmm_free_frame(phys);
            paging_destroy_address_space(pd, pd_phys);
            return -1;
        }

        void *kpage = paging_temp_map_frame(pd, phys);
        if (!kpage)
        {
            paging_destroy_address_space(pd, pd_phys);
            return -1;
        }

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
    {
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }

    if (paging_map_page(pd, trampoline_addr, tramp_phys,
                        PAGE_PRESENT | PAGE_RW | PAGE_USER) != 0)
    {
        pmm_free_frame(tramp_phys);
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }
    void *tramp_kpage = paging_temp_map_frame(pd, tramp_phys);
    if (!tramp_kpage)
    {
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }
    u32 initial_stack;
    u32 user_argv;
    if (exec_build_stack(pd, argc, argv, &initial_stack, &user_argv) != 0)
    {
        paging_temp_unmap_frame(pd);
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }
    tramp_kpage = paging_temp_map_frame(pd, tramp_phys);
    if (!tramp_kpage)
    {
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }

    unsigned char tramp_code[] = {
        0x68, 0, 0, 0, 0,             /* push argv */
        0x68, 0, 0, 0, 0,             /* push argc */
        0xB8, 0, 0, 0, 0,             /* mov eax, entry */
        0xFF, 0xD0,                   /* call eax */
        0x89, 0xC3,                   /* mov ebx, eax */
        0xB8, 0x01, 0, 0, 0,          /* mov eax, SYS_exit */
        0xCD, 0x80                    /* int 0x80 */
    };

    memcpy(&tramp_code[1], &user_argv, sizeof(user_argv));
    memcpy(&tramp_code[6], &argc, sizeof(u32));
    memcpy(&tramp_code[11], &entry, sizeof(u32));

    memcpy(tramp_kpage, tramp_code, sizeof(tramp_code));
    /* zero the rest of the page */
    if (sizeof(tramp_code) < PAGE_SIZE)
        memset((char *)tramp_kpage + sizeof(tramp_code), 0, PAGE_SIZE - sizeof(tramp_code));

    paging_temp_unmap_frame(pd);

    user_exec_context_t *ctx = malloc(sizeof(*ctx));
    if (!ctx)
    {
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }

    ctx->entry = entry;
    /* Use the trampoline as the initial eip so returns from user code
     * are handled by the trampoline which will exit the process. */
    ctx->entry = trampoline_addr;
    ctx->stack_top = initial_stack;
    ctx->argc = (u32)argc;
    ctx->argv = user_argv;

    u8 *kernel_stack = malloc(4096);
    if (!kernel_stack)
    {
        free(ctx);
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }

    Thread *thread = thread_create(user_thread_entry, ctx, kernel_stack, 4096);
    if (!thread)
    {
        free(ctx);
        free(kernel_stack);
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }

    process_t *process = proc_create();
    if (!process)
    {
        free(thread);
        free(ctx);
        free(kernel_stack);
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }
    process->pd = pd;
    process->pd_phys = pd_phys;

    KLOG_INFO("exec_elf_context: created user thread %p for process %p entry=0x%x stack_top=0x%x",
              thread, process, entry, ctx->stack_top);

    proc_attach_thread(process, thread);
    sched_add(thread);

    return process->pid;
}

int exec_elf_vnode(struct vnode *vn)
{
    int result = exec_elf_vnode_args(vn, 0, NULL);
    return result < 0 ? result : 0;
}

int exec_elf_vnode_args(struct vnode *vn, int argc,
                        const char *const argv[])
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
        paging_destroy_address_space(pd, pd_phys);
        return -1;
    }

    KLOG_INFO("exec_elf_vnode: loaded entry=0x%x", entry);
    return exec_elf_context(pd, pd_phys, entry, argc, argv);
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
    int result = exec_elf_context(pd, pd_phys, entry, 0, NULL);
    return result < 0 ? result : 0;
}