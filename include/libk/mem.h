
#ifndef MEM_H
#define MEM_H

#include <stddef.h>
#include <stdint.h>
#include "libk/types.h"

/**
 * @brief Fill the first n bytes of dest with the constant byte c.
 * @return dest
 */
void *memset(void *dest, int c, usize n);

/**
 * @brief Copy n bytes from src to dest. Behavior is undefined for overlapping regions.
 * @return dest
 */
void *memcpy(void *dest, const void *src, usize n);

/**
 * @brief Copy n bytes from src to dest. Correctly handles overlapping regions.
 * @return dest
 */
void *memmove(void *dest, const void *src, usize n);

/**
 * @brief Compare n bytes between s1 and s2.
 * @return 0 if equal, <0 if s1<s2, >0 if s1>s2
 */
int memcmp(const void *s1, const void *s2, usize n);

/**
 * @brief Scan the first n bytes of the memory area pointed to by s for the first
 * instance of c. Returns pointer to the matching byte or NULL if not found.
 */
void *memchr(const void *s, int c, usize n);

#endif