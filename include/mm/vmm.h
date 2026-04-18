/**
 * @author J.Fairweather
 * @file vmm.h
 * @brief Virtual Memory Manager (VMM) — page lookup, mapping, and address‑space creation.
 *
 * The VMM provides higher‑level helpers on top of the paging subsystem:
 * - retrieving or creating page table entries
 * - mapping and unmapping virtual pages
 * - switching page directories
 * - creating new address spaces for processes
 *
 * It does not manage physical memory directly; that is handled by the PMM.
 */

#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include "mm/paging.h"
#include "libk/types.h"

/**
 * @important
 * - `page_entry_t` is defined in paging.h
 * - `struct page_directory` is defined in paging.h
 * - Do **not** redefine them here
 */

/**
 * @typedef page_directory_t
 * @brief Alias for the paging subsystem's page directory type.
 */
typedef struct page_directory page_directory_t;

/**
 * @struct page_table
 * @brief Represents a single page table containing 1024 page entries.
 */
struct page_table
{
    page_entry_t entries[1024];
};

/**
 * @typedef page_table_t
 * @brief Alias for the page table structure.
 */
typedef struct page_table page_table_t;

/* -------------------------------------------------------------------------- */
/*  Virtual Memory Manager API                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Identity‑map the first portion of memory in the given page directory.
 *
 * Typically used during early kernel initialization to ensure low memory
 * addresses remain valid after paging is enabled.
 *
 * @param pd Page directory to modify.
 */
void vmm_init_identity(page_directory_t *pd);

/**
 * @brief Retrieve the page entry for a virtual address.
 *
 * If @p make is non‑zero, missing page tables will be created automatically.
 *
 * @param vaddr Virtual address to look up.
 * @param make  Whether to create page tables if missing.
 * @param dir   Page directory to search.
 * @return Pointer to the page entry, or NULL if not found and @p make == 0.
 */
page_entry_t *vmm_get_page(u32 vaddr, int make, page_directory_t *dir);

/**
 * @brief Map a physical page to a virtual address.
 *
 * Creates page tables as needed and sets the appropriate flags.
 *
 * @param phys  Physical address of the page.
 * @param virt  Virtual address to map to.
 * @param flags Page flags (PAGE_PRESENT, PAGE_RW, PAGE_USER).
 * @param dir   Page directory to modify.
 */
void vmm_map_page(u32 phys, u32 virt, u32 flags, page_directory_t *dir);

/**
 * @brief Unmap a virtual page.
 *
 * Does not free the underlying physical frame — that is the PMM's job.
 *
 * @param virt Virtual address to unmap.
 * @param dir  Page directory to modify.
 */
void vmm_unmap_page(u32 virt, page_directory_t *dir);

/**
 * @brief Switch to a new page directory.
 *
 * Loads CR3 with the physical address of @p dir.
 *
 * @param dir Page directory to activate.
 */
void switch_page_directory(page_directory_t *dir);

/**
 * @brief Create a new, empty page directory.
 *
 * Allocates a new address space suitable for a process.
 * Kernel mappings are typically copied or shared depending on your design.
 *
 * @return Pointer to the new page directory.
 */
page_directory_t *vmm_create_directory(void);

#endif /* VMM_H */
