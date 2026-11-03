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
#define SYS_clear 43
#define SYS_ftruncate 44
#define SYS_setcolor 45

enum minios_color
{
    MINIOS_COLOR_BLACK = 0,
    MINIOS_COLOR_BLUE = 1,
    MINIOS_COLOR_GREEN = 2,
    MINIOS_COLOR_CYAN = 3,
    MINIOS_COLOR_RED = 4,
    MINIOS_COLOR_MAGENTA = 5,
    MINIOS_COLOR_BROWN = 6,
    MINIOS_COLOR_LIGHT_GREY = 7,
    MINIOS_COLOR_DARK_GREY = 8,
    MINIOS_COLOR_LIGHT_BLUE = 9,
    MINIOS_COLOR_LIGHT_GREEN = 10,
    MINIOS_COLOR_LIGHT_CYAN = 11,
    MINIOS_COLOR_LIGHT_RED = 12,
    MINIOS_COLOR_LIGHT_MAGENTA = 13,
    MINIOS_COLOR_YELLOW = 14,
    MINIOS_COLOR_WHITE = 15
};

#define MINIOS_O_RDONLY 0x1
#define MINIOS_O_WRONLY 0x2
#define MINIOS_O_RDWR   (MINIOS_O_RDONLY | MINIOS_O_WRONLY)
#define MINIOS_O_CREATE 0x4
#define MINIOS_SEEK_SET 0
#define MINIOS_SEEK_CUR 1
#define MINIOS_SEEK_END 2

/* --------------------------------------------------------------------------
 * User-space libc wrappers
 * -------------------------------------------------------------------------- */
int print(const char *s);
int printcolor(const char *s, int foreground, int background);
int setcolor(int foreground, int background);
int scan(char *buf, int max);
int clear_screen(void);
int write(int fd, const void *buf, int count);
int read(int fd, void *buf, int count);
int open(const char *path, int flags);
int close(int fd);
int getpid(void);
int chdir(const char *path);
int getcwd(char *buffer, int size);
int mkdir(const char *path);
int rmdir(const char *path);
int unlink(const char *path);
int lseek(int fd, int offset, int whence);
int ftruncate(int fd, int length);
int execve(const char *path, char *const argv[]);
int waitpid(int pid, int *status, int options);
int readdir(int fd, int index, void *out);
void exit(int status);

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