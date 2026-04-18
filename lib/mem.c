#include "libk/types.h"
#include <stddef.h>

void *memset(void *dest, int c, usize n)
{
    unsigned char *p = (unsigned char *)dest;
    unsigned char v = (unsigned char)c;
    for (usize i = 0; i < n; ++i)
        p[i] = v;
    return dest;
}

void *memcpy(void *dest, const void *src, usize n)
{
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (usize i = 0; i < n; ++i)
        d[i] = s[i];
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