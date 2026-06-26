#include "libk/types.h"
#include <stddef.h>
#include "libk/mem.h"

void *memset(void *dest, int c, usize n)
{
    unsigned char *p = (unsigned char *)dest;
    unsigned char v = (unsigned char)c;
    usize i = 0;
    // Handle unaligned start
    while (i < n && (uintptr_t)(p + i) % sizeof(uintptr_t) != 0)
    {
        p[i++] = v;
    }
    // Set word by word if possible
    if (i < n)
    {
        uintptr_t wv = 0;
        for (usize j = 0; j < sizeof(uintptr_t); ++j)
        {
            wv |= (uintptr_t)v << (j * 8);
        }
        uintptr_t *wp = (uintptr_t *)(p + i);
        usize words = (n - i) / sizeof(uintptr_t);
        for (usize j = 0; j < words; ++j)
        {
            wp[j] = wv;
        }
        i += words * sizeof(uintptr_t);
    }
    // Remaining bytes
    while (i < n)
    {
        p[i++] = v;
    }
    return dest;
}

void *memcpy(void *dest, const void *src, usize n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    // If both aligned to uintptr_t, copy word by word
    if (n >= sizeof(uintptr_t) && (uintptr_t)d % sizeof(uintptr_t) == 0 && (uintptr_t)s % sizeof(uintptr_t) == 0)
    {
        uintptr_t *dw = (uintptr_t *)d;
        const uintptr_t *sw = (const uintptr_t *)s;
        usize words = n / sizeof(uintptr_t);
        for (usize i = 0; i < words; ++i)
        {
            dw[i] = sw[i];
        }
        usize rem = n % sizeof(uintptr_t);
        d += words * sizeof(uintptr_t);
        s += words * sizeof(uintptr_t);
        for (usize i = 0; i < rem; ++i)
        {
            d[i] = s[i];
        }
    }
    else
    {
        for (usize i = 0; i < n; ++i)
        {
            d[i] = s[i];
        }
    }
    return dest;
}

void *memmove(void *dest, const void *src, usize n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    if (d < s)
    {
        for (usize i = 0; i < n; ++i)
            d[i] = s[i];
    }
    else if (d > s)
    {
        for (usize i = n; i > 0; --i)
            d[i - 1] = s[i - 1];
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, usize n)
{
    const unsigned char *a = (const unsigned char *)s1;
    const unsigned char *b = (const unsigned char *)s2;
    for (usize i = 0; i < n; ++i)
    {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
    }
    return 0;
}

void *memchr(const void *s, int c, usize n)
{
    const unsigned char *p = (const unsigned char *)s;
    unsigned char uc = (unsigned char)c;
    for (usize i = 0; i < n; ++i)
    {
        if (p[i] == uc)
            return (void *)(p + i);
    }
    return NULL;
}