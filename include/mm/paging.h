/**
 * @file paging.h
 * @brief x86 paging subsystem: page tables, mapping, and address-space management.
 *
 * This module provides the kernel’s paging interface, including:
 * - page directory structures
 * - page table entry flags
 * - mapping/unmapping of virtual pages
 * - switching address spaces
 * - invalidating TLB entries
 *
 * It forms the foundation of the virtual memory manager (VMM).
 */

#ifndef MM_PAGING_H
#define MM_PAGING_H

#include "libk/types.h"

/**
 * @name Paging Constants
 * @{
 */
#define PAGE_SIZE 0x1000u   /**< Size of a page: 4 KiB. */
#define PAGE_PRESENT 0x001u /**< Page is present in memory. */
#define PAGE_RW 0x002u      /**< Read/write permission. */
#define PAGE_USER 0x004u    /**< User-mode accessible. */
/** @} */

/**
 * @typedef page_entry_t
 * @brief 32-bit page table or page directory entry.
 */
typedef u32 page_entry_t;

/**
 * @struct page_directory
 * @brief Represents a 4 KiB-aligned page directory containing 1024 entries.
 *
 * Each entry points to a page table or a 4 MiB page (if PSE is enabled).
 */
struct page_directory
{
    u32 entries[1024] __attribute__((aligned(4096)));
};

struct address_space {
    u32 phys;
    struct page_directory *virt;
};

/**
 * @brief Initialize the paging subsystem.
 *
 * Sets up the kernel page directory, identity-maps essential regions,
 * and enables paging on the CPU.
 */
void paging_init(void);

/**
 * @brief Physical address of the kernel page directory.
 *
 * Set during paging initialization.
 */
extern u32 kernel_pd_phys;

/**
 * @brief Virtual pointer to the kernel page directory.
 */
extern struct page_directory *kernel_pd;

/**
 * @brief Create a new address space (page directory).
 *
 * Allocates and initializes a new page directory suitable for a process.
 *
 * @param out_phys Output: physical address of the new page directory.
 * @return Pointer to the new page directory.
 */
struct page_directory *paging_create_address_space(u32 *out_phys);

/**
 * @brief Switch to a page directory using its physical address.
 *
 * Loads CR3 with @p pd_phys.
 */
void paging_switch_phys(u32 pd_phys);

/**
 * @brief Switch to a page directory using its virtual pointer.
 *
 * Convenience wrapper around paging_switch_phys().
 */
void paging_switch(struct page_directory *pd);

/**
 * @brief Temporarily map a physical frame into the kernel address space.
 */
void *paging_temp_map_frame(struct page_directory *pd, u32 phys);
void paging_temp_unmap_frame(struct page_directory *pd);

/**
 * @brief Map a single 4 KiB page.
 */
void paging_map_page(struct page_directory *pd, u32 virt, u32 phys, u32 flags);

/**
 * @brief Invalidate a single TLB entry.
 */
static inline void invlpg(void *addr)
{
    __asm__ volatile("invlpg (%0)" ::"r"(addr) : "memory");
}

#endif /* MM_PAGING_H */