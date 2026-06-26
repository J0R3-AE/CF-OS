/* file: src/kheap.c */
#include "mm/heap.h"
#include "mm/vmm.h"
#include "mm/pmm.h"

#include "libk/errno.h"
#include "libk/string.h"
#include "libk/mem.h"

// Simple heap allocator using a linked list of free/used chunks.
typedef struct heap_chunk
{
    usize size;              // size of the payload (not including the header)
    int free;                // flag to indicate if this chunk is free or used
    struct heap_chunk *prev; // pointer to the previous chunk in the list
    struct heap_chunk *next; // pointer to the next chunk in the list
} heap_chunk_t;

static heap_chunk_t *heap_base = NULL;
static u32 heap_start_addr = 0;
static u32 heap_end_addr = 0;
static u32 heap_max_addr = 0;
static page_directory_t *heap_pd = NULL;

static Errno_t expand_heap_by_pages(u32 pages)
{
    for (u32 i = 0; i < pages; ++i)
    {
        if (heap_end_addr + PAGE_SIZE > heap_max_addr)
            return ERR_HEAP_EXHAUSTED;

        if (heap_pd)
        {
            u32 f = pmm_alloc_frame();
            if (!f)
                return ERR_FRAME_UNAVAILABLE;

            vmm_map_page(f, heap_end_addr, PAGE_RW, heap_pd);
        }
        else
        {
            /* Early boot: use identity-mapped heap region before paging is enabled. */
            u32 phys = heap_end_addr;
            pmm_reserve_region(phys, phys + PAGE_SIZE);
        }

        void *vaddr = (void *)(uintptr_t)heap_end_addr;
        memset(vaddr, 0, PAGE_SIZE);
        heap_end_addr += PAGE_SIZE;
    }

    return ERR_SUCCESS;
}

void heap_init(u32 heap_start, u32 heap_max, page_directory_t *dir)
{
    heap_start_addr = heap_start;
    heap_end_addr = heap_start;
    heap_max_addr = heap_max;
    heap_pd = dir;
    heap_base = NULL;
}

/* find a suitable free chunk with first-fit */
static heap_chunk_t *find_free_chunk(usize size)
{
    heap_chunk_t *cur = heap_base;
    while (cur)
    {
        if (cur->free && cur->size >= size)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

static heap_chunk_t *request_space(usize size)
{
    usize total = size + sizeof(heap_chunk_t);
    u32 needed_pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;

    Errno_t rc = expand_heap_by_pages(needed_pages);
    if (rc != ERR_SUCCESS)
        return NULL;

    heap_chunk_t *chunk =
        (heap_chunk_t *)(uintptr_t)(heap_end_addr - needed_pages * PAGE_SIZE);

    chunk->size = size;
    chunk->free = 0;
    chunk->prev = NULL;
    chunk->next = NULL;

    if (!heap_base)
    {
        heap_base = chunk;
    }
    else
    {
        heap_chunk_t *cur = heap_base;
        while (cur->next)
            cur = cur->next;

        cur->next = chunk;
        chunk->prev = cur;
    }

    return chunk;
}

void *malloc(usize size)
{
    if (size == 0)
        return NULL;

    heap_chunk_t *chunk = find_free_chunk(size);
    if (chunk)
    {
        chunk->free = 0;
        return (void *)((uintptr_t)chunk + sizeof(heap_chunk_t));
    }

    chunk = request_space(size);
    if (!chunk)
        return NULL;

    return (void *)((uintptr_t)chunk + sizeof(heap_chunk_t));
}

void free(void *ptr)
{
    if (!ptr)
        return;

    heap_chunk_t *chunk = (heap_chunk_t *)((uintptr_t)ptr - sizeof(heap_chunk_t));
    chunk->free = 1;

    if (chunk->next && chunk->next->free)
    {
        chunk->size += sizeof(heap_chunk_t) + chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next)
            chunk->next->prev = chunk;
    }

    if (chunk->prev && chunk->prev->free)
    {
        chunk->prev->size += sizeof(heap_chunk_t) + chunk->size;
        chunk->prev->next = chunk->next;
        if (chunk->next)
            chunk->next->prev = chunk->prev;
    }
}

Errno_t free_align(void *ptr)
{
    if (!ptr)
        return ERR_INVALID_ARGUMENT;

    void *raw = ((void **)ptr)[-1];
    free(raw);
    return ERR_SUCCESS;
}

/* Return usable payload size for a pointer returned by malloc */
usize malloc_usable_size(void *ptr)
{
    if (!ptr)
        return 0;

    heap_chunk_t *chunk = (heap_chunk_t *)((uintptr_t)ptr - sizeof(heap_chunk_t));
    return chunk->size;
}

/* calloc: allocate nmemb * size bytes and zero them */
void *calloc(usize nmemb, usize size)
{
    if (nmemb == 0 || size == 0)
        return NULL;

    if (nmemb > (SIZE_MAX / size))
        return NULL;

    usize total = nmemb * size;
    void *p = malloc(total);
    if (!p)
        return NULL;

    memset(p, 0, total);
    return p;
}

/* realloc: grow or shrink an allocation; preserves contents up to min(old,new) */
void *realloc(void *ptr, usize new_size)
{
    if (!ptr)
        return malloc(new_size);

    if (new_size == 0)
    {
        free(ptr);
        return NULL;
    }

    heap_chunk_t *chunk = (heap_chunk_t *)((uintptr_t)ptr - sizeof(heap_chunk_t));
    usize old_size = chunk->size;

    if (new_size <= old_size)
        return ptr;

    void *new_ptr = malloc(new_size);
    if (!new_ptr)
        return NULL;

    memcpy(new_ptr, ptr, old_size);
    free(ptr);
    return new_ptr;
}

void *malloc_align(usize size, usize align)
{
    if (align == 0 || (align & (align - 1)) != 0)
        return NULL;

    usize total = size + align - 1 + sizeof(void *);
    void *raw = malloc(total);
    if (!raw)
        return NULL;

    uintptr_t raw_addr = (uintptr_t)raw + sizeof(void *);
    u32 aligned = align_up((u32)raw_addr, (u32)align);
    void **store = (void **)(uintptr_t)(aligned - sizeof(void *));
    *store = raw;
    return (void *)(uintptr_t)aligned;
}