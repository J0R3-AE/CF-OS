/* file: src/pmm.c */

#include "mm/pmm.h"

#include "libk/string.h"
#include "libk/mem.h"
#include "libk/types.h"
#include "libk/math.h"

static u32 *frame_bitmap = 0; /* bitset: 1 = used, 0 = free */
static u32 total_frames_count = 0;
static u32 frames_used = 0;

/* helpers */

// Mark a frame as used in the bitmap
static inline void set_frame(u32 frame_addr)
{
    u32 frame = frame_addr / PMM_FRAME_SIZE;
    frame_bitmap[frame >> 5] |= (1u << (frame & 31u));
}

// Mark a frame as free in the bitmap
static inline void clear_frame(u32 frame_addr)
{
    u32 frame = frame_addr / PMM_FRAME_SIZE;
    frame_bitmap[frame >> 5] &= ~(1u << (frame & 31u));
}

// Check if a frame is marked as used in the bitmap
static inline int test_frame(u32 frame_addr)
{
    u32 frame = frame_addr / PMM_FRAME_SIZE;
    return (frame_bitmap[frame >> 5] & (1u << (frame & 31u))) != 0;
}

// Find the first free frame in the bitmap and return its index, or -1 if none are free.
static int first_free_frame(void)
{
    u32 len = (total_frames_count + 31) >> 5;
    for (u32 i = 0; i < len; ++i)
    {
        if (frame_bitmap[i] != 0xFFFFFFFFu)
        {
            for (int b = 0; b < 32; ++b)
            {
                if ((frame_bitmap[i] & (1u << b)) == 0)
                {
                    u32 frame = (i << 5) + b;
                    if (frame < total_frames_count) return frame;
                    return -1;
                }
            }
        }
    }

    return -1;
}

//
void pmm_init(u32 mem_size_bytes, u32 kernel_end_paddr)
{
    total_frames_count = mem_size_bytes / PMM_FRAME_SIZE;

    u32 bitmap_size_bytes = ((total_frames_count + 31) >> 5) * 4u;

    u32 bitmap_phys = align_up(kernel_end_paddr, PMM_FRAME_SIZE);

    frame_bitmap = (u32 *)(uptr)bitmap_phys;                 /* treat as physical pointer - integrate with your identity mapping */

    /* wipe bitmap */
    memset((void *)frame_bitmap, 0, bitmap_size_bytes);

    /* reserve frames used by kernel bitmap and below */
    u32 reserved_end = bitmap_phys + bitmap_size_bytes;
    for (u32 addr = 0; addr < reserved_end; addr += PMM_FRAME_SIZE)
    {
        set_frame(addr);
        frames_used++;
    }
}

u32 pmm_alloc_frame(void)
{
    int frame = first_free_frame();

    if (frame == -1) return 0; /* 0 indicates failure (frame 0 often reserved) */

    u32 addr = (u32)frame * PMM_FRAME_SIZE;
    
    set_frame(addr);
    ++frames_used;

    return addr;
}

void pmm_free_frame(u32 paddr)
{
    if (!test_frame(paddr))
        return; /* double free ignored */
    clear_frame(paddr);
    --frames_used;
}

void pmm_reserve_region(u32 paddr_start, u32 paddr_end)
{
    paddr_start = align_down(paddr_start, PMM_FRAME_SIZE);
    paddr_end = align_up(paddr_end, PMM_FRAME_SIZE);
    for (u32 a = paddr_start; a < paddr_end; a += PMM_FRAME_SIZE)
    {
        if (!test_frame(a))
        {
            set_frame(a);
            ++frames_used;
        }
    }
}

u32 pmm_total_frames(void) { return total_frames_count; }              // Return the total number of frames available (based on the memory size).
u32 pmm_used_frames(void) { return frames_used; }                      // Return the number of frames currently allocated/used.
u32 pmm_free_frames(void) { return total_frames_count - frames_used; } // Return the number of free frames available for allocation.