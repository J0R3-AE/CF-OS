#ifndef _SYSCALL_H
#define _SYSCALL_H

/* ============================================================================
 * MiniOS Syscall ABI
 * ============================================================================
 * Rules:
 *  - NEVER reorder existing syscall numbers
 *  - NEVER reuse deleted numbers
 *  - Always append new syscalls at the end
 *  - Keep kernel + libc in sync with this file
 * ========================================================================== */

#include "libc/string.h"
#include "libc/mem.h"
#include "libc/log.h"
#include "libc/types.h"
#include "libc/errno.h"
#include "libc/dirent.h"

#include "kernel/arch/idt.h"
#include "kernel/arch/io.h"

#include "kernel/fs/fd.h"
#include "kernel/fs/vfs.h"

#include "kernel/drivers/tty.h"
#include "kernel/drivers/line.h"
#include "kernel/drivers/serial.h"

#include "kernel/sched/scheduler.h"
#include "kernel/sched/thread.h"
#include "kernel/proc/process.h"
/* --------------------------------------------------------------------------
 * Core process control
 * -------------------------------------------------------------------------- */
#define SYS_exit        1
#define SYS_fork        2
#define SYS_execve      8
#define SYS_waitpid     7

#define SYS_getpid      16
#define SYS_getppid     17

/* --------------------------------------------------------------------------
 * File descriptors (POSIX-like)
 * -------------------------------------------------------------------------- */
#define SYS_read        3
#define SYS_write       4
#define SYS_open        5
#define SYS_close       6
#define SYS_lseek       15

/* --------------------------------------------------------------------------
 * Directory / filesystem (VFS expansion zone)
 * -------------------------------------------------------------------------- */
#define SYS_readdir     20
#define SYS_chdir       9
#define SYS_getcwd      10
#define SYS_mkdir       11
#define SYS_rmdir       12
#define SYS_unlink      13
#define SYS_stat        14

/* --------------------------------------------------------------------------
 * Memory management
 * -------------------------------------------------------------------------- */
#define SYS_brk         18
#define SYS_sbrk        19

#define SYS_print 40
#define SYS_scan 41
#define SYS_printcolor 42

/* TODO: move per‑process fd tables into process_t */
static inline fd_table_t g_fd_tables[64];
static inline int g_fd_tables_used[64];

static inline fd_table_t *syscall_get_fd_table(void)
{
    process_t *p = proc_current();
    if (!p || p->pid < 0 || p->pid >= 64)
        return NULL;

    if (!g_fd_tables_used[p->pid])
    {
        fd_table_init(&g_fd_tables[p->pid]);
        g_fd_tables_used[p->pid] = 1;
    }

    return &g_fd_tables[p->pid];
}


/* --------------------------------------------------------------------------
 * User-space libc wrappers
 * -------------------------------------------------------------------------- */
int print(const char *s);
int scan(char *buf, int max);
int write(int fd, const void *buf, int count);
int read(int fd, void *buf, int count);
int open(const char *path, int flags);
int close(int fd);
int getpid(void);
int execve(const char *path, char *const argv[]);
int readdir(int fd, int index, void *out);
void exit(int status);
int shell(void);

/* --------------------------------------------------------------------------
 * Future reserved range (DO NOT USE YET)
 * -------------------------------------------------------------------------- */
/*
#define SYS_socket      30
#define SYS_bind        31
#define SYS_mount       32
#define SYS_umount      33
#define SYS_ioctl       34
#define SYS_dup         35
#define SYS_pipe        36
*/

#endif