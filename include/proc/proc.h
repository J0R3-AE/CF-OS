/**
 * @file proc.h
 * @brief Process abstraction and ELF execution interface.
 *
 * This header defines the minimal process structure used by the kernel
 * along with the entry point for loading and executing ELF binaries
 * from the VFS. The process model is intentionally lightweight and
 * designed for a single‑address‑space or early‑stage multitasking kernel.
 */

#ifndef PROC_H
#define PROC_H

#include "libk/types.h"
#include "mm/paging.h"
#include "fs/fs_types.h"
#include "elf/elf.h"

/**
 * @struct process
 * @brief Represents a user process and its associated address space.
 *
 * This structure contains the essential metadata required to load and
 * execute an ELF binary. It includes the page directory (both virtual
 * and physical forms), the entry point extracted from the ELF header,
 * and the initial user stack pointer.
 *
 * The kernel currently assumes:
 *   - One page directory per process
 *   - Identity mapping for kernel space
 *   - A single user stack allocated at exec time
 */
typedef struct process {
    struct page_directory *pd;       /**< Virtual pointer to page directory (identity‑mapped). */
    struct page_directory *pd_virt;  /**< Virtual pointer for higher‑half or remapped access. */
    u32 pd_phys;                     /**< Physical address of page directory (loaded into CR3). */

    Elf32_Addr entry;                /**< ELF entry point address. */
    u32 user_stack;                  /**< Initial user stack pointer. */
} process_t;

/**
 * @brief Load and execute an ELF binary from a vnode.
 *
 * This function performs:
 *   - ELF header validation
 *   - Segment loading into a new address space
 *   - Page directory creation and CR3 switch
 *   - User stack setup
 *   - Transfer of control to the ELF entry point
 *
 * @param vn VFS vnode containing the ELF binary.
 * @return 0 on success, negative value on failure.
 *
 * @note This version does not support argv/envp; the process starts
 *       with a clean stack and no arguments.
 */
int exec_elf_vnode(struct vnode *vn);
int exec_path(const char *path);

#endif /* PROC_H */
