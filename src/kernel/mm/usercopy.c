#include "kernel/mm/usercopy.h"

#include "kernel/mm/paging.h"
#include "kernel/proc/proc.h"
#include "libc/errno.h"
#include "libc/mem.h"
#include "libc/string.h"

static struct page_directory *current_address_space(void)
{
    process_t *process = proc_current();
    return process ? process->pd : NULL;
}

int copy_from_user(void *kernel_dst, const void *user_src, usize length)
{
    u32 address = (u32)(uintptr_t)user_src;
    if ((!kernel_dst || !user_src) && length)
        return ERR_BAD_ADDRESS;

    if (!paging_user_range_valid(current_address_space(), address, length,
                                 false))
        return ERR_BAD_ADDRESS;

    if (length)
        memcpy(kernel_dst, user_src, length);
    return ERR_SUCCESS;
}

int copy_to_user(void *user_dst, const void *kernel_src, usize length)
{
    u32 address = (u32)(uintptr_t)user_dst;
    if ((!user_dst || !kernel_src) && length)
        return ERR_BAD_ADDRESS;

    if (!paging_user_range_valid(current_address_space(), address, length,
                                 true))
        return ERR_BAD_ADDRESS;

    if (length)
        memcpy(user_dst, kernel_src, length);
    return ERR_SUCCESS;
}

int copy_user_string(char *kernel_dst, usize capacity, const char *user_src)
{
    if (!kernel_dst || !capacity || !user_src)
        return ERR_BAD_ADDRESS;

    u32 source = (u32)(uintptr_t)user_src;
    for (usize i = 0; i < capacity; i++)
    {
        u64 address = (u64)source + i;
        if (address > 0xFFFFFFFFULL)
            return ERR_BAD_ADDRESS;

        int result = copy_from_user(
            &kernel_dst[i], (const void *)(uintptr_t)(u32)address, 1);
        if (result != ERR_SUCCESS)
            return result;
        if (kernel_dst[i] == '\0')
            return ERR_SUCCESS;
    }

    kernel_dst[capacity - 1] = '\0';
    return ERR_BUFFER_TOO_SMALL;
}
