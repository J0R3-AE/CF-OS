#include "syscall.h"
#include <stdint.h>

static inline int syscall(int num, int a, int b, int c)
{
    int ret;
    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(num), "b"(a), "c"(b), "d"(c)
        : "memory"
    );
    return ret;
}

int print(const char *s)
{
    return syscall(SYS_print, (int)(uintptr_t)s, 0, 0);
}

int scan(char *buf, int max)
{
    return syscall(SYS_scan, (int)(uintptr_t)buf, max, 0);
}

int write(int fd, const void *buf, int count)
{
    return syscall(SYS_write, fd, (int)(uintptr_t)buf, count);
}

int read(int fd, void *buf, int count)
{
    return syscall(SYS_read, fd, (int)(uintptr_t)buf, count);
}

int open(const char *path, int flags)
{
    return syscall(SYS_open, (int)(uintptr_t)path, flags, 0);
}

int close(int fd)
{
    return syscall(SYS_close, fd, 0, 0);
}

int getpid(void)
{
    return syscall(SYS_getpid, 0, 0, 0);
}

int execve(const char *path, char *const argv[])
{
    return syscall(SYS_execve, (int)(uintptr_t)path, (int)(uintptr_t)argv, 0);
}

int readdir(int fd, int index, void *out)
{
    return syscall(SYS_readdir, fd, index, (int)(uintptr_t)out);
}

void exit(int status)
{
    syscall(SYS_exit, status, 0, 0);
    for (;;)
        ;
}
