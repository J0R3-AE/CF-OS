/* file: src/pmm.c */

#include "mm/pmm.h"

#include "libk/errno.h"
#include "libk/string.h"
#include "libk/mem.h"
#include "libk/types.h"

static u32 *frame_bitmap = NULL; /* bitset: 1 = used, 0 = free */
static u32 total_frames_count = 0;
static u32 frames_used = 0;

/// @section "Frame Management"
static inline void set_frame(u32 frame_addr)
{
    u32 frame = frame_addr / PMM_FRAME_SIZE;
    frame_bitmap[frame >> 5] |= (1u << (frame & 31u));
}

static inline void clear_frame(u32 frame_addr)
{
    u32 frame = frame_addr / PMM_FRAME_SIZE;
    frame_bitmap[frame >> 5] &= ~(1u << (frame & 31u));
}

static inline int test_frame(u32 frame_addr)
{
    u32 frame = frame_addr / PMM_FRAME_SIZE;
    return (frame_bitmap[frame >> 5] & (1u << (frame & 31u))) != 0;
}

static int first_free_frame(void)
{
    u32 len = (total_frames_count + 31) >> 5;

    for (u32 i = 0; i < len; ++i)
    {
        if (frame_bitmap[i] != 0xFFFFFFFFu)
        {
            for (u32 b = 0; b < 32; ++b)
            {
                if ((frame_bitmap[i] & (1u << b)) == 0)
                {
                    u32 frame = (i << 5) + b;

                    if (frame < total_frames_count)
                        return (int)frame;

                    return -1;
                }
            }
        }
    }

    return -1;
}

/// @section "Initialization"
Errno_t pmm_init(u32 mem_size_bytes, u32 kernel_end_paddr)
{
    if (mem_size_bytes == 0)
        return ERR_INVALID_ARGUMENT;

    total_frames_count = mem_size_bytes / PMM_FRAME_SIZE;

    u32 bitmap_size_bytes =
        ((total_frames_count + 31) >> 5) * sizeof(u32);

    u32 bitmap_phys =
        align_up(kernel_end_paddr, PMM_FRAME_SIZE);

    frame_bitmap = (u32 *)(uptr)bitmap_phys;

    if (!frame_bitmap)
        return ERR_BAD_ADDRESS;

    memset(frame_bitmap, 0, bitmap_size_bytes);

    frames_used = 0;

    /* Reserve everything up to the end of the bitmap */
    u32 reserved_end = bitmap_phys + bitmap_size_bytes;

    for (u32 addr = 0; addr < reserved_end; addr += PMM_FRAME_SIZE)
    {
        set_frame(addr);
        ++frames_used;
    }

    return ERR_SUCCESS;
}

/// @section "Frame Allocation and Freeing" 
u32 pmm_alloc_frame(void)
{
    int frame = first_free_frame();

    if (frame < 0)
        return 0;

    u32 addr = (u32)frame * PMM_FRAME_SIZE;

    set_frame(addr);
    ++frames_used;

    return addr;
}

Errno_t pmm_free_frame(u32 paddr)
{
    if (paddr & (PMM_FRAME_SIZE - 1))
        return ERR_INVALID_ALIGNMENT;

    if (!test_frame(paddr))
        return ERR_INVALID_ARGUMENT;

    clear_frame(paddr);

    if (frames_used)
        --frames_used;

    return ERR_SUCCESS;
}

/// @section "Region Reservation" 
Errno_t pmm_reserve_region(u32 paddr_start, u32 paddr_end)
{
    if (paddr_start >= paddr_end)
        return ERR_INVALID_ARGUMENT;

    paddr_start = align_down(paddr_start, PMM_FRAME_SIZE);
    paddr_end   = align_up(paddr_end, PMM_FRAME_SIZE);

    for (u32 a = paddr_start; a < paddr_end; a += PMM_FRAME_SIZE)
    {
        if (!test_frame(a))
        {
            set_frame(a);
            ++frames_used;
        }
    }

    return ERR_SUCCESS;
}

Errno_t pmm_unreserve_region(u32 paddr_start, u32 paddr_end)
{
    if (paddr_start >= paddr_end)
        return ERR_INVALID_ARGUMENT;

    paddr_start = align_down(paddr_start, PMM_FRAME_SIZE);
    paddr_end   = align_up(paddr_end, PMM_FRAME_SIZE);

    for (u32 a = paddr_start; a < paddr_end; a += PMM_FRAME_SIZE)
    {
        if (test_frame(a))
        {
            clear_frame(a);

            if (frames_used)
                --frames_used;
        }
    }

    return ERR_SUCCESS;
}

/// @section "Frame Count Queries"
u32 pmm_total_frames(void)
{
    return total_frames_count;
}

u32 pmm_used_frames(void)
{
    return frames_used;
}

u32 pmm_free_frames(void)
{
    return total_frames_count - frames_used;
}