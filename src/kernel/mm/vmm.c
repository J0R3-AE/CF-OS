/* file: src/vmm.c */
#include "mm/vmm.h"
#include "mm/pmm.h"

#include "libk/string.h"
#include "libk/mem.h"

#include <stddef.h>

static inline void *phys_to_virt(u32 p) { return (void *)(uintptr_t)p; } // Convert physical address to virtual address (identity mapping in this simple implementation)
static inline u32 virt_to_phys(void *v) { return (u32)(uintptr_t)v; }    // Convert virtual address to physical address (identity mapping in this simple implementation)

// Get the page entry for a given virtual address, optionally creating page tables if 'make' is true.
page_entry_t *vmm_get_page(u32 vaddr, int make, page_directory_t *dir)
{
    u32 pd_index = (vaddr >> 22) & 0x3FFu;
    u32 pt_index = (vaddr >> 12) & 0x3FFu;
    page_entry_t pd_entry = dir->entries[pd_index];

    if (pd_entry & PAGE_PRESENT)
    {
        page_table_t *pt = (page_table_t *)phys_to_virt(pd_entry & 0xFFFFF000u);
        return &pt->entries[pt_index];
    }

    else if (make)
    {
        /* allocate a page table */
        u32 frame = pmm_alloc_frame();

        if (!frame) return NULL;

        memset((void *)phys_to_virt(frame), 0, PAGE_SIZE);
        dir->entries[pd_index] = (frame & 0xFFFFF000u) | PAGE_PRESENT | PAGE_RW;

        page_table_t *pt = (page_table_t *)phys_to_virt(frame);

        return &pt->entries[pt_index];
    }
    else return NULL;
}

// Map a virtual address to a physical address with given flags in the specified page directory.
void vmm_map_page(u32 phys, u32 virt, u32 flags, page_directory_t *dir)
{
    page_entry_t *entry = vmm_get_page(virt, 1, dir);

    if (!entry) return;

    *entry = (phys & 0xFFFFF000u) | (flags & 0xFFFu) | PAGE_PRESENT;

    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

// Unmap a virtual address in the specified page directory.
void vmm_unmap_page(u32 virt, page_directory_t *dir)
{
    page_entry_t *entry = vmm_get_page(virt, 0, dir);

    if (!entry) return;

    *entry = 0;
    __asm__ volatile("invlpg (%0)" ::"r"(virt) : "memory");
}

// Switch to a new page directory (used for context switching or enabling paging).
void switch_page_directory(page_directory_t *dir)
{
    u32 phys = virt_to_phys((void *)dir);

    __asm__ volatile("mov %0, %%eax\n\tmov %%eax, %%cr3" ::"r"(phys));
}

// Create a new page directory, initialized to zero (no mappings).
page_directory_t *vmm_create_directory(void)
{
    u32 frame = pmm_alloc_frame();

    if (!frame) return NULL;

    page_directory_t *dir = (page_directory_t *)phys_to_virt(frame);
    memset(dir, 0, PAGE_SIZE);

    return dir;
}

void vmm_init_identity(page_directory_t *pd)
{
    /* create identity mapping for first X MB where kernel lives; caller can map more */
    /* This is a helper — use vmm_map_page for real mappings */
}