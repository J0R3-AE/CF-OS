/**
 * @file kheap.h
 * @brief Kernel heap allocator (malloc/free/realloc) backed by virtual memory.
 *
 * This module implements a simple kernel heap using a linked list of free
 * blocks. It supports dynamic allocation, freeing, coalescing adjacent blocks,
 * and optional aligned allocation. The heap grows via virtual memory mappings
 * provided by the kernel's paging subsystem.
 */

#ifndef KHEAP_H
#define KHEAP_H

#include <stdint.h>
#include <stddef.h>
#include "mm/vmm.h"

/**
 * @brief Initialize the kernel heap.
 *
 * Sets up the heap region and prepares internal bookkeeping structures.
 * The heap grows using the provided page directory when additional pages
 * need to be mapped.
 *
 * @param heap_start Virtual start address of the heap.
 * @param heap_max   Maximum virtual address the heap may grow to.
 * @param dir        Page directory used for mapping new heap pages.
 */
void heap_init(u32 heap_start, u32 heap_max, page_directory_t *dir);

/**
 * @brief Allocate a block of memory.
 *
 * Allocates at least @p size bytes and returns a pointer to usable memory.
 * The returned pointer is aligned to at least `sizeof(void*)`.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void *malloc(usize size);

/**
 * @brief Free a previously allocated block.
 *
 * Releases the memory pointed to by @p ptr and merges adjacent free blocks
 * to reduce fragmentation.
 *
 * @param ptr Pointer returned by malloc/calloc/realloc.
 */
void free(void *ptr);

/**
 * @brief Allocate memory with a specific alignment.
 *
 * @warning Not implemented yet.
 *
 * @param size  Number of bytes to allocate.
 * @param align Alignment requirement (must be a power of two).
 * @return Pointer to aligned memory, or NULL.
 */
void *malloc_align(usize size, usize align);

/**
 * @brief Resize an allocated memory block.
 *
 * Behaves like:
 * - `malloc(new_size)` if @p ptr is NULL
 * - `free(ptr)` and returns NULL if @p new_size is 0
 * - Otherwise allocates a new block, copies old data, frees old block,
 *   and returns the new pointer.
 *
 * @param ptr      Existing allocation or NULL.
 * @param new_size New size in bytes.
 * @return Pointer to resized block, or NULL on failure.
 */
void *realloc(void *ptr, usize new_size);

/**
 * @brief Allocate and zero‑initialize memory.
 *
 * Allocates `nmemb * size` bytes and sets them to zero.
 *
 * @param nmemb Number of elements.
 * @param size  Size of each element.
 * @return Pointer to zero‑initialized memory, or NULL.
 */
void *calloc(usize nmemb, usize size);

#endif /* KHEAP_H */
