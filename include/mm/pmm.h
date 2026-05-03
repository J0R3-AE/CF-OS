/**
 * @file pmm.h
 * @brief Physical Memory Manager (PMM) — frame allocation and reservation.
 *
 * The PMM manages physical memory in fixed-size 4 KiB frames. It tracks which
 * frames are free, used, or reserved, and provides allocation primitives used
 * by the kernel and paging subsystem.
 */

#ifndef PMM_H
#define PMM_H

#include "libk/types.h"

/**
 * @def PMM_FRAME_SIZE
 * @brief Size of a physical frame (4 KiB).
 */
#define PMM_FRAME_SIZE 0x1000u

/**
 * @brief Initialize the physical memory manager.
 *
 * Sets up the PMM bitmap based on the total memory size and marks the kernel
 * region as used so it cannot be overwritten.
 *
 * @param mem_size_bytes   Total physical memory size in bytes.
 * @param kernel_end_paddr Physical end address of the kernel image.
 */
void pmm_init(u32 mem_size_bytes, u32 kernel_end_paddr);

/**
 * @brief Free a previously allocated physical frame.
 *
 * Marks the frame at @p paddr as free and available for allocation.
 *
 * @param paddr Physical address of the frame to free (must be 4 KiB aligned).
 */
void pmm_free_frame(u32 paddr);

/**
 * @brief Reserve a region of physical memory.
 *
 * Marks all frames in the range `[paddr_start, paddr_end)` as used.
 * Useful for ACPI tables, MMIO regions, bootloader structures, etc.
 *
 * @param paddr_start Start of the reserved region (inclusive).
 * @param paddr_end   End of the reserved region (exclusive).
 */
void pmm_reserve_region(u32 paddr_start, u32 paddr_end);

/**
 * @brief Allocate a single free physical frame.
 *
 * @return Physical address of the allocated frame, or 0 if none are available.
 */
u32 pmm_alloc_frame(void);

/**
 * @brief Get the total number of physical frames.
 *
 * @return Total frame count based on memory size.
 */
u32 pmm_total_frames(void);

/**
 * @brief Get the number of frames currently allocated.
 *
 * @return Number of frames marked as used.
 */
u32 pmm_used_frames(void);

/**
 * @brief Get the number of free frames available.
 *
 * @return Number of free frames.
 */
u32 pmm_free_frames(void);

#endif /* PMM_H */
