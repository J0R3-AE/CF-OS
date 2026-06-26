/**
 * @file pmm.h
 * @brief Physical Memory Manager (PMM) — frame allocation and reservation.
 *
 * Manages physical memory using 4 KiB frames with a bitmap allocator.
 * Used by paging, heap, and kernel subsystems.
 */

#ifndef PMM_H
#define PMM_H

#include "libk/types.h"
#include "libk/errno.h"

/* =========================
 * Constants
 * ========================= */

#define PMM_FRAME_SIZE 0x1000u  /* 4 KiB frames */

/* =========================
 * API
 * ========================= */

/**
 * @brief Initialize PMM.
 *
 * Builds frame bitmap and reserves kernel + metadata regions.
 *
 * @param mem_size_bytes Total physical memory in bytes.
 * @param kernel_end_paddr End of kernel in physical memory.
 * @return Errno_t status code.
 */
Errno_t pmm_init(u32 mem_size_bytes, u32 kernel_end_paddr);

/**
 * @brief Allocate a physical frame.
 *
 * @return Physical address of frame, or 0 on failure.
 *
 * NOTE:
 * 0 is safe as an error value because physical frame 0 is reserved.
 */
u32 pmm_alloc_frame(void);

/**
 * @brief Free a physical frame.
 *
 * @param paddr Frame address (must be 4 KiB aligned).
 */
Errno_t pmm_free_frame(u32 paddr);

/**
 * @brief Reserve a physical memory region.
 *
 * Marks all frames in range [start, end) as used.
 *
 * @param paddr_start Start address (inclusive).
 * @param paddr_end   End address (exclusive).
 */
Errno_t pmm_reserve_region(u32 paddr_start, u32 paddr_end);

/* =========================
 * Stats
 * ========================= */

u32 pmm_total_frames(void);
u32 pmm_used_frames(void);
u32 pmm_free_frames(void);

#endif /* PMM_H */