#include "shared/syscall.h"
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

<<<<<<<< HEAD:src/shared/syscall.c
========
int printcolor(const char *s, int foreground, int background)
{
    return syscall(SYS_printcolor, (int)(uintptr_t)s, foreground, background);
}

int setcolor(int foreground, int background)
{
    return syscall(SYS_setcolor, foreground, background, 0);
}

int scan(char *buf, int max)
{
    return syscall(SYS_scan, (int)(uintptr_t)buf, max, 0);
}

int clear_screen(void)
{
    return syscall(SYS_clear, 0, 0, 0);
}

>>>>>>>> 0314c3a (	modified:   Makefile):src/shared/libc/syscall.c
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

int chdir(const char *path)
{
    return syscall(SYS_chdir, (int)(uintptr_t)path, 0, 0);
}

int getcwd(char *buffer, int size)
{
    return syscall(SYS_getcwd, (int)(uintptr_t)buffer, size, 0);
}

int mkdir(const char *path)
{
    return syscall(SYS_mkdir, (int)(uintptr_t)path, 0, 0);
}

int rmdir(const char *path)
{
    return syscall(SYS_rmdir, (int)(uintptr_t)path, 0, 0);
}

int unlink(const char *path)
{
    return syscall(SYS_unlink, (int)(uintptr_t)path, 0, 0);
}

int lseek(int fd, int offset, int whence)
{
    return syscall(SYS_lseek, fd, offset, whence);
}

int ftruncate(int fd, int length)
{
    return syscall(SYS_ftruncate, fd, length, 0);
}

int execve(const char *path, char *const argv[])
{
    return syscall(SYS_execve, (int)(uintptr_t)path, (int)(uintptr_t)argv, 0);
}

int waitpid(int pid, int *status, int options)
{
    return syscall(SYS_waitpid, pid, (int)(uintptr_t)status, options);
}

int readdir(int fd, int index, void *out)
{
    return syscall(SYS_readdir, fd, index, (int)(uintptr_t)out);
}

int chdir(const char *path)
{
    return syscall(SYS_chdir, (int)(uintptr_t)path, 0, 0);
}

int getcwd(char *buf, int size)
{
    return syscall(SYS_getcwd, (int)(uintptr_t)buf, size, 0);
}

void exit(int status)
{
    syscall(SYS_exit, status, 0, 0);
    for (;;)
        ;
}
