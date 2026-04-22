#include "mm/paging.h"
#include "mm/heap.h"
#include "libk/types.h"
#include "libk/string.h"

#define TEMP_MAP_VADDR 0xFEE00000

u32 kernel_pd_phys;
struct page_directory *kernel_pd;

/* low-level helpers */

static inline void load_cr3(u32 phys)
{
    __asm__ volatile("mov %0, %%cr3" : : "r"(phys) : "memory");
}

static inline void enable_paging(void)
{
    u32 cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= (1u << 31);
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

static void identity_map_range(struct page_directory *pd, u32 map_bytes)
{
    u32 num_pages = (map_bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    u32 num_pt = (num_pages + 1023) / 1024;

    for (u32 pd_index = 0; pd_index < num_pt; pd_index++)
    {
        u32 pt_phys = pmm_alloc_frame();
        if (!pt_phys)
            return;

        u32 *pt = (u32 *)pt_phys; // identity-mapped
        memset(pt, 0, PAGE_SIZE);

        u32 base_page = pd_index * 1024;
        for (u32 i = 0; i < 1024; i++)
        {
            u32 page = base_page + i;
            if (page >= num_pages)
                break;
            u32 phys = page * PAGE_SIZE;
            pt[i] = phys | PAGE_PRESENT | PAGE_RW;
        }

        pd->entries[pd_index] = pt_phys | PAGE_PRESENT | PAGE_RW;
    }
}

void paging_init(void)
{
    kernel_pd_phys = pmm_alloc_frame();
    kernel_pd = (struct page_directory *)kernel_pd_phys;

    memset(kernel_pd, 0, sizeof(*kernel_pd));

    identity_map_range(kernel_pd, 16 * 1024 * 1024);

    for (u32 addr = 0xF0000000; addr < 0xF0100000; addr += PAGE_SIZE)
        paging_map_page(kernel_pd, addr, addr, PAGE_PRESENT | PAGE_RW);

    for (u32 addr = 0xFD000000; addr < 0xFD400000; addr += PAGE_SIZE)
        paging_map_page(kernel_pd, addr, addr, PAGE_PRESENT | PAGE_RW);

    load_cr3(kernel_pd_phys);
    enable_paging();
}

// paging.c
struct page_directory *paging_create_user_pd(u32 *out_phys)
{
    u32 pd_phys = pmm_alloc_frame();
    if (!pd_phys)
        return NULL;

    struct page_directory *pd = (void *)pd_phys;
    memset(pd, 0, PAGE_SIZE);

    // copy ONLY kernel space (top 1GB)
    for (int i = 768; i < 1024; i++)
    {
        pd->entries[i] = kernel_pd->entries[i];
    }

    if (out_phys)
        *out_phys = pd_phys;

    return pd;
}

void paging_switch_phys(u32 pd_phys)
{
    if (!pd_phys)
        return;
    load_cr3(pd_phys);
}

void paging_switch(struct page_directory *pd)
{
    if (!pd)
        return;
    // pd is identity-mapped, so its virtual address == physical
    load_cr3((u32)pd);
}

void paging_map_page(struct page_directory *pd, u32 virt, u32 phys, u32 flags)
{
    u32 pd_index = (virt >> 22) & 0x3FF;
    u32 pt_index = (virt >> 12) & 0x3FF;

    u32 pde = pd->entries[pd_index];
    u32 *pt;

    if (!(pde & PAGE_PRESENT))
    {
        u32 pt_phys = pmm_alloc_frame();
        if (!pt_phys)
            return;

        pt = (u32 *)pt_phys; // identity-mapped
        memset(pt, 0, PAGE_SIZE);

        u32 pde_flags = PAGE_PRESENT | PAGE_RW;
        if (flags & PAGE_USER)
            pde_flags |= PAGE_USER;

        pd->entries[pd_index] = pt_phys | pde_flags;
    }
    else
    {
        pt = (u32 *)(pde & ~0xFFFu);
        if (flags & PAGE_USER)
            pd->entries[pd_index] |= PAGE_USER;
    }

    pt[pt_index] = (phys & ~0xFFFu) | (flags & (PAGE_PRESENT | PAGE_RW | PAGE_USER));
    invlpg((void *)virt);
}

static u32 temp_map_phys = 0;

void *paging_temp_map_frame(struct page_directory *pd, u32 phys)
{
    (void)pd;

    // reuse single slot (simple, non-reentrant)
    paging_map_page(kernel_pd, TEMP_MAP_VADDR, phys, PAGE_PRESENT | PAGE_RW);

    temp_map_phys = phys;
    return (void *)TEMP_MAP_VADDR;
}

void paging_temp_unmap_frame(struct page_directory *pd)
{
    (void)pd;

    // clear mapping
    paging_map_page(kernel_pd, TEMP_MAP_VADDR, 0, 0);

    temp_map_phys = 0;
}