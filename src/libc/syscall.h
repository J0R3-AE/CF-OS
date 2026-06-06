#ifndef _SYSCALL_H
#define _SYSCALL_H

#include <stdint.h>
#include "dirent.h"

/* ============================================================================
 * MiniOS libc syscall interface
 * - Mirrors kernel ABI exactly
 * - Provides safe userland wrappers
 * ========================================================================== */

/* Syscall numbers (MUST MATCH KERNEL ABI) */
#define SYS_exit     1
#define SYS_fork     2
#define SYS_read     3
#define SYS_write    4
#define SYS_open     5
#define SYS_close    6
#define SYS_waitpid  7
#define SYS_execve   8
#define SYS_chdir    9
#define SYS_getcwd   10
#define SYS_mkdir    11
#define SYS_rmdir    12
#define SYS_unlink   13
#define SYS_stat     14
#define SYS_lseek    15
#define SYS_getpid   16
#define SYS_getppid  17
#define SYS_brk      18
#define SYS_sbrk     19
#define SYS_readdir  20

/* Low-level syscall entry */
int syscall(int num, int a, int b, int c);

/* ============================================================================
 * libc wrappers (USER API)
 * ========================================================================== */

static inline int write(int fd, const void *buf, int count)
{
    return syscall(SYS_write, fd, (int)(uintptr_t)buf, count);
}

static inline int read(int fd, void *buf, int count)
{
    return syscall(SYS_read, fd, (int)(uintptr_t)buf, count);
}

static inline int open(const char *path, int flags, int mode)
{
    return syscall(SYS_open, (int)(uintptr_t)path, flags, mode);
}

static inline int close(int fd)
{
    return syscall(SYS_close, fd, 0, 0);
}

/* Your current VFS design: indexed readdir */
static inline int readdir(int fd, int index, dirent_t *out)
{
    return syscall(SYS_readdir, fd, index, (int)out);
}

#endif