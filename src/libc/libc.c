/**
 * user/libc/libc.c - Basic C library for user mode
 * Implements syscall wrappers and standard library functions
 */

#include "syscall.h"
#include <stdarg.h>
#include <stddef.h>

/* ============================================================================
 * Syscall Wrappers
 * ============================================================================ */

void exit(int status) {
    syscall(SYS_exit, status, 0, 0);
    while (1);  // Should never reach here
}

int write(int fd, const void *buf, int count) {
    return syscall(SYS_write, fd, (int)buf, count);
}

int read(int fd, void *buf, int count) {
    return syscall(SYS_read, fd, (int)buf, count);
}

int open(const char *path, int flags, int mode) {
    return syscall(SYS_open, (int)path, flags, mode);
}

int close(int fd) {
    return syscall(SYS_close, fd, 0, 0);
}

int getpid(void) {
    // TODO: not yet available in kernel
    return -1;
}

int fork(void) {
    // TODO: not yet available in kernel
    return -1;
}

int waitpid(int pid, int *status, int options) {
    // TODO: not yet available in kernel
    (void)pid; (void)status; (void)options;
    return -1;
}

int execve(const char *filename, char *const argv[], char *const envp[]) {
    // TODO: not yet available in kernel
    (void)filename; (void)argv; (void)envp;
    return -1;
}

/* ============================================================================
 * String Functions
 * ============================================================================ */

int strlen(const char *s) {
    int len = 0;
    while (s[len])
        len++;
    return len;
}

char *strcpy(char *dst, const char *src) {
    while ((*dst++ = *src++));
    return dst - strlen(src) - 1;
}

char *strcat(char *dst, const char *src) {
    while (*dst)
        dst++;
    strcpy(dst, src);
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (int)*s1 - (int)*s2;
}

int strncmp(const char *s1, const char *s2, int n) {
    while (n-- > 0 && *s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    if (n < 0)
        return 0;
    return (int)*s1 - (int)*s2;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return (char *)NULL;
}

char *strrchr(const char *s, int c) {
    const char *p = NULL;
    while (*s) {
        if (*s == (char)c)
            p = s;
        s++;
    }
    return (char *)p;
}

/* ============================================================================
 * Memory Functions
 * ============================================================================ */

void *memset(void *s, int c, int n) {
    unsigned char *p = (unsigned char *)s;
    while (n-- > 0)
        *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dst, const void *src, int n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n-- > 0)
        *d++ = *s++;
    return dst;
}

int memcmp(const void *s1, const void *s2, int n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    while (n-- > 0) {
        if (*p1 != *p2)
            return (int)*p1 - (int)*p2;
        p1++;
        p2++;
    }
    return 0;
}

/* Simple heap management */
static char *heap_ptr = (char *)0x10000000;
static char *heap_limit = (char *)0x10000000 + 0x100000;  // 1MB

void *malloc(int size) {
    if (heap_ptr + size > heap_limit)
        return NULL;
    void *ptr = (void *)heap_ptr;
    heap_ptr += size;
    return ptr;
}

void free(void *ptr) {
    // No-op for now - simple heap allocator
    (void)ptr;
}

/* ============================================================================
 * Character/Number Functions
 * ============================================================================ */

int isdigit(int c) {
    return c >= '0' && c <= '9';
}

int isalpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 'A';
    return c;
}

int tolower(int c) {
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 'a';
    return c;
}

/* ============================================================================
 * Printf Implementation
 * ============================================================================ */

static void putc(char c) {
    write(1, &c, 1);
}

static void puts_internal(const char *s) {
    while (*s)
        putc(*s++);
}

static void puti(int n) {
    if (n < 0) {
        putc('-');
        n = -n;
    }

    if (n == 0) {
        putc('0');
        return;
    }

    char buf[32];
    int idx = 0;
    while (n > 0) {
        buf[idx++] = '0' + (n % 10);
        n /= 10;
    }

    while (idx-- > 0)
        putc(buf[idx]);
}

static void putx(int n) {
    const char *hex = "0123456789abcdef";
    if (n == 0) {
        putc('0');
        return;
    }

    char buf[16];
    int idx = 0;
    while (n > 0) {
        buf[idx++] = hex[n & 0xf];
        n >>= 4;
    }

    while (idx-- > 0)
        putc(buf[idx]);
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
                case 's':
                    puts_internal(va_arg(ap, char *));
                    break;
                case 'd':
                case 'i':
                    puti(va_arg(ap, int));
                    break;
                case 'x':
                    putx(va_arg(ap, int));
                    break;
                case 'c':
                    putc((char)va_arg(ap, int));
                    break;
                case '%':
                    putc('%');
                    break;
                default:
                    putc('%');
                    putc(*fmt);
                    break;
            }
            fmt++;
        } else if (*fmt == '\\' && *(fmt + 1) == 'n') {
            putc('\n');
            fmt += 2;
        } else {
            putc(*fmt++);
        }
    }

    va_end(ap);
    return 0;
}

int puts(const char *s) {
    puts_internal(s);
    putc('\n');
    return 0;
}

/* ============================================================================
 * File I/O Functions
 * ============================================================================ */

typedef struct {
    int fd;
    int mode;
} FILE;

FILE *stdin_obj = NULL;
FILE *stdout_obj = NULL;
FILE *stderr_obj = NULL;

FILE *fopen(const char *path, const char *mode) {
    int flags = 0;
    
    if (mode[0] == 'r')
        flags = 0;  // TODO: define O_RDONLY
    else if (mode[0] == 'w')
        flags = 1;  // TODO: define O_WRONLY
    else
        return NULL;

    int fd = open(path, flags, 0);
    if (fd < 0)
        return NULL;

    FILE *f = malloc(sizeof(FILE));
    if (!f) {
        close(fd);
        return NULL;
    }

    f->fd = fd;
    f->mode = flags;
    return f;
}

int fclose(FILE *f) {
    if (!f)
        return -1;
    int ret = close(f->fd);
    free(f);
    return ret;
}

int fwrite(const void *ptr, int size, int count, FILE *f) {
    if (!f)
        return 0;
    return write(f->fd, ptr, size * count) / size;
}

int fread(void *ptr, int size, int count, FILE *f) {
    if (!f)
        return 0;
    return read(f->fd, ptr, size * count) / size;
}

int fprintf(FILE *f, const char *fmt, ...) {
    // TODO: implement proper fprintf
    (void)f; (void)fmt;
    return 0;
}

int getchar(void) {
    char c = 0;
    int ret = read(0, &c, 1);
    return ret > 0 ? (int)c : -1;
}

int putchar(int c) {
    char ch = (char)c;
    write(1, &ch, 1);
    return c;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

void abort(void) {
    exit(1);
}

int atoi(const char *s) {
    int n = 0;
    while (*s && isdigit(*s)) {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}

char *getenv(const char *name) {
    // TODO: implement environment variables
    (void)name;
    return NULL;
}
