/* file: src/kheap.c */
#include "mm/heap.h"
#include "mm/pmm.h"

#include "libk/string.h"
#include "libk/mem.h"
#include "libk/math.h"
#include "libk/log.h"

// Simple heap allocator using a linked list of free/used chunks. This is a very basic implementation and can be improved in many ways (e.g. better free chunk management, splitting/merging chunks, support for aligned allocations, etc.). For simplicity, it expands the heap by allocating pages from the physical memory manager as needed.
typedef struct heap_chunk
{
    usize size;              // size of the payload (not including the header)
    int free;                // flag to indicate if this chunk is free or used
    struct heap_chunk *prev; // pointer to the previous chunk in the list (can be NULL for the first chunk)
    struct heap_chunk *next; // pointer to the next chunk in the list (can be NULL for the last chunk)
} heap_chunk_t;

static heap_chunk_t *heap_base = NULL;   // pointer to the first chunk in the heap; this is the entry point for finding free chunks and managing the heap
static u32 heap_start_addr = 0;          // starting virtual address of the heap region; this is used to track the current end of the heap and to allocate new chunks by expanding the heap
static u32 heap_end_addr = 0;            // current end of the heap region; this is updated as new chunks are allocated and the heap grows; it should never exceed heap_max_addr
static u32 heap_max_addr = 0;            // maximum virtual address of the heap region; this is set during initialization and should not be exceeded by heap_end_addr; if the heap needs to grow beyond this, allocation should fail
static page_directory_t *heap_pd = NULL; // pointer to the page directory used for the heap region; this is needed to map new pages when expanding the heap; it should be set during initialization and used for all heap-related mappings

static int expand_heap_by_pages(u32 pages)
{
    for (u32 i = 0; i < pages; ++i)
    {
        if (heap_end_addr + PAGE_SIZE > heap_max_addr)
        {
            KLOG_ERROR("expand_heap_by_pages: heap max exceeded");
            return -1;
        }

        if (heap_pd)
        {
            u32 f = pmm_alloc_frame();
            if (!f)
            {
                KLOG_ERROR("expand_heap_by_pages: failed to allocate frame for heap expansion");
                return -1;
            }
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
    KLOG_OKAY("expand_heap_by_pages: expanded heap by %u pages, new end address = 0x%x", pages, heap_end_addr);
    return 0;
}

void heap_init(u32 heap_start, u32 heap_max, page_directory_t *dir)
{
    heap_start_addr = heap_start;
    heap_end_addr = heap_start;
    heap_max_addr = heap_max;
    heap_pd = dir;
    heap_base = NULL;
    KLOG_INFO("heap_init: heap initialized with start=0x%x max=0x%x", heap_start_addr, heap_max_addr);
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
        KLOG_INFO("find_free_chunk: checking chunk at %p (size=%u free=%d)", cur, cur ? cur->size : 0, cur ? cur->free : 0);
    }
    KLOG_WARN("find_free_chunk: no suitable free chunk found for size %u", size);
    return NULL;
}

static heap_chunk_t *request_space(usize size)
{
    /* ensure there is space by expanding heap */
    usize total = size + sizeof(heap_chunk_t);
    u32 needed_pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    if (expand_heap_by_pages(needed_pages) < 0)
    {
        KLOG_ERROR("request_space: failed to expand heap for size %u", size);
        return NULL;
    }

    heap_chunk_t *chunk = (heap_chunk_t *)(uintptr_t)(heap_end_addr - needed_pages * PAGE_SIZE);
    chunk->size = size;
    chunk->free = 0;
    chunk->prev = NULL;
    chunk->next = NULL;
    /* append to list */
    if (!heap_base)
    {
        KLOG_INFO("request_space: allocated first chunk at %p (size=%u)", chunk, size);
        heap_base = chunk;
    }
    else
    {
        heap_chunk_t *cur = heap_base;
        while (cur->next)
            cur = cur->next;
        cur->next = chunk;
        chunk->prev = cur;
        KLOG_INFO("request_space: allocated new chunk at %p (size=%u)", chunk, size);
    }
    KLOG_OKAY("request_space: allocated new chunk at %p (size=%u)", chunk, size);
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
        KLOG_WARN("malloc: reusing free chunk at %p for size %u", chunk, size);
        return (void *)((uintptr_t)chunk + sizeof(heap_chunk_t));
    }
    chunk = request_space(size);
    if (!chunk)
    {
        KLOG_ERROR("malloc: failed to allocate chunk for size %u", size);
        return NULL;
    }
    KLOG_OKAY("malloc: allocated new chunk at %p for size %u", chunk, size);
    return (void *)((uintptr_t)chunk + sizeof(heap_chunk_t));
}

void free(void *ptr)
{
    if (!ptr)
        return;
    heap_chunk_t *chunk = (heap_chunk_t *)((uintptr_t)ptr - sizeof(heap_chunk_t));
    chunk->free = 1;
    /* coalescing left and right */
    if (chunk->next && chunk->next->free)
    {
        chunk->size += sizeof(heap_chunk_t) + chunk->next->size;
        chunk->next = chunk->next->next;
        if (chunk->next)
        {
            chunk->next->prev = chunk;
        }
        KLOG_INFO("free: coalesced with next chunk at %p, new size = %u", chunk->next, chunk->size);
    }
    if (chunk->prev && chunk->prev->free)
    {
        chunk->prev->size += sizeof(heap_chunk_t) + chunk->size;
        chunk->prev->next = chunk->next;
        if (chunk->next)
        {
            chunk->next->prev = chunk->prev;
        }
        KLOG_INFO("free: coalesced with previous chunk at %p, new size = %u", chunk->prev, chunk->prev->size);
    }
}

void free_align(void *ptr)
{
    if (!ptr)
    {
        KLOG_WARN("free_align: null pointer passed to free_align");
        return;
    }
    void *raw = ((void **)ptr)[-1];
    free(raw);
    KLOG_OKAY("free_align: deallocated aligned memory at %p", ptr);
}

/* Return usable payload size for a pointer returned by kmalloc */
usize malloc_usable_size(void *ptr)
{
    if (!ptr)
        return 0;
    typedef struct heap_chunk
    {
        usize size;
        int free;
        struct heap_chunk *prev;
        struct heap_chunk *next;
    } heap_chunk_t;
    heap_chunk_t *chunk = (heap_chunk_t *)((uintptr_t)ptr - sizeof(heap_chunk_t));
    return chunk->size;
} /* calloc: allocate nmemb * size bytes and zero them */
void *calloc(usize nmemb, usize size)
{
    if (nmemb == 0 || size == 0)
        return NULL; /* overflow check */
    if (size != 0 && nmemb > (SIZE_MAX / size))
        return NULL;
    usize total = nmemb * size;
    void *p = malloc(total);
    if (!p)
        return NULL; /* zero memory */
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
    } /* read old size from heap header */
    typedef struct heap_chunk
    {
        usize size;
        int free;
        struct heap_chunk *prev;
        struct heap_chunk *next;
    } heap_chunk_t;
    heap_chunk_t *chunk = (heap_chunk_t *)((uintptr_t)ptr - sizeof(heap_chunk_t));
    usize old_size = chunk->size; /* if new size fits in old block, keep same pointer */
    if (new_size <= old_size)
    { /* optional: could shrink metadata, but keep simple */
        return ptr;
    } /* allocate new block and copy */
    void *new_ptr = malloc(new_size);
    if (!new_ptr)
        return NULL; /* copy old contents */
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