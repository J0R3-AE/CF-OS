#ifndef _SYSCALL_H
#define _SYSCALL_H

// System call numbers
#define SYS_exit    1
#define SYS_write   4
#define SYS_read    3
#define SYS_open    5
#define SYS_close   6

// Syscall function
int syscall(int num, int a, int b, int c);

#endif