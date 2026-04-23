/* Minimal, freestanding string helpers
 * - No libc dependencies
 * - Only uses standard integer/size types
 * - Safe, simple implementations suitable for kernel or freestanding use
 */

#include <stddef.h>
#include <stdint.h>

#include "mm/heap.h"
#include "libk/mem.h"

usize strlen(const char *s)
{
    const char *p = s;
    while (*p)
        ++p;
    return (usize)(p - s);
}

char *strcpy(char *dest, const char *src)
{
    char *d = dest;
    while ((*d++ = *src++))
        ;
    return dest;
}

char *strncpy(char *dest, const char *src, usize n)
{
    char *d = dest;
    usize i = 0;
    for (; i < n && src[i]; ++i)
        d[i] = src[i];
    for (; i < n; ++i)
        d[i] = '\0';
    return dest;
}

char *strchr(const char *s, int c)
{
    char ch = (char)c;
    while (*s)
    {
        if (*s == ch)
            return (char *)s;
        ++s;
    }
    return NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    char ch = (char)c;
    while (*s)
    {
        if (*s == ch)
            last = s;
        ++s;
    }
    return (char *)last;
}

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b))
    {
        ++a;
        ++b;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

int strncmp(const char *a, const char *b, usize n)
{
    for (usize i = 0; i < n; ++i)
    {
        if (a[i] != b[i])
            return (unsigned char)a[i] - (unsigned char)b[i];
        if (a[i] == '\0')
            return 0;
    }
    return 0;
}

static char tolower_ascii(char c)
{
    if (c >= 'A' && c <= 'Z')
        return (char)(c - 'A' + 'a');
    return c;
}

int strcasecmp(const char *a, const char *b)
{
    while (*a && *b)
    {
        char ca = tolower_ascii(*a++);
        char cb = tolower_ascii(*b++);
        if (ca != cb)
            return (int)(unsigned char)ca - (int)(unsigned char)cb;
    }
    return (int)(unsigned char)tolower_ascii(*a) - (int)(unsigned char)tolower_ascii(*b);
}

char *strstr(const char *haystack, const char *needle)
{
    if (!*needle)
        return (char *)haystack;
    for (; *haystack; ++haystack)
    {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && (*h == *n))
        {
            ++h;
            ++n;
        }
        if (!*n)
            return (char *)haystack;
    }
    return NULL;
}

char *strcat(char *dest, const char *src)
{
    char *d = dest;
    while (*d)
        ++d;
    while ((*d++ = *src++))
        ;
    return dest;
}

char *strncat(char *dest, const char *src, usize n)
{
    char *d = dest;
    while (*d)
        ++d;
    usize i = 0;
    for (; i < n && src[i]; ++i)
        d[i] = src[i];
    d[i] = '\0';
    return dest;
}

usize strspn(const char *s, const char *accept)
{
    usize count = 0;
    for (; *s; ++s)
    {
        const char *a = accept;
        int matched = 0;
        for (; *a; ++a)
            if (*a == *s)
            {
                matched = 1;
                break;
            }
        if (!matched)
            break;
        ++count;
    }
    return count;
}

usize strcspn(const char *s, const char *reject)
{
    usize count = 0;
    for (; *s; ++s)
    {
        const char *r = reject;
        int matched = 0;
        for (; *r; ++r)
            if (*r == *s)
            {
                matched = 1;
                break;
            }
        if (matched)
            break;
        ++count;
    }
    return count;
}

char *strdup(const char *src)
{
    usize len = strlen(src) + 1; // String plus '\0'
    char *dst = malloc(len);     // Allocate space
    if (dst == NULL)
        return NULL;       // No memory
    memcpy(dst, src, len); // Copy the block
    return dst;            // Return the new string
}

char *strndup(const char *src, usize maxlen)
{
    if (!src)
        return NULL;
    usize n = 0;
    while (n < maxlen && src[n])
        ++n;
    char *dst = (char *)malloc(n + 1);
    if (!dst)
        return NULL;
    memcpy(dst, src, n);
    dst[n] = '\0';
    return dst;
}

char *strtok(char *s, const char *delim)
{
    char *p;
    if (s)
        p = s;

    if (!p)
        return NULL;

    while (*p && strchr(delim, *p))
        p++;

    if (!*p)
        return NULL;

    char *start = p;

    while (*p && !strchr(delim, *p))
        p++;

    if (*p)
    {
        *p = '\0';
        p++;
    }

    return start;
}