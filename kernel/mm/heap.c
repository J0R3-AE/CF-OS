/* file: src/kheap.c */
#include "mm/heap.h"
#include "mm/pmm.h"

#include "libk/string.h"
#include "libk/mem.h"

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
        u32 f = pmm_alloc_frame();
        if (!f)
            return -1;
        vmm_map_page(f, heap_end_addr, PAGE_RW, heap_pd);
        memset((void *)(uintptr_t)heap_end_addr, 0, PAGE_SIZE);
        heap_end_addr += PAGE_SIZE;
    }
    return 0;
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
    /* ensure there is space by expanding heap */
    usize total = size + sizeof(heap_chunk_t);
    u32 needed_pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    if (expand_heap_by_pages(needed_pages) < 0)
        return NULL;
    heap_chunk_t *chunk = (heap_chunk_t *)(uintptr_t)(heap_end_addr - needed_pages * PAGE_SIZE);
    chunk->size = size;
    chunk->free = 0;
    chunk->prev = NULL;
    chunk->next = NULL;
    /* append to list */
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
    /* coalescing left and right */
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

void free_align(void *ptr)
{
    if (!ptr)
        return;
    void *raw = ((void **)ptr)[-1];
    free(raw);
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
        return NULL; /* zero memory without relying on libc */
    unsigned char *b = (unsigned char *)p;
    for (usize i = 0; i < total; ++i)
        b[i] = 0;
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
    unsigned char *d = (unsigned char *)new_ptr;
    unsigned char *s = (unsigned char *)ptr;
    for (usize i = 0; i < old_size; ++i)
        d[i] = s[i];
    free(ptr);
    return new_ptr;
}

void *malloc_align(usize size, usize align)
{
    /* not implemented: simple wrapper for now */
    return malloc(size);
}