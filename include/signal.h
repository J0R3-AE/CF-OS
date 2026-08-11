/* include/signal.h
 *
 * Minimal POSIX-like signal definitions and sigset helpers for kernel + userland.
 * This header is intentionally small; expand as you implement sigaction, sigreturn, etc.
 */

#ifndef _KERNEL_SIGNAL_H
#define _KERNEL_SIGNAL_H

#include "libc/types.h"

/* POSIX-like signal numbers (use familiar values for userland compatibility) */
typedef enum {
    SIGHUP    = 1,
    SIGINT    = 2,
    SIGQUIT   = 3,
    SIGILL    = 4,
    SIGTRAP   = 5,
    SIGABRT   = 6,
    SIGBUS    = 7,
    SIGFPE    = 8,
    SIGKILL   = 9,   /* cannot be caught/ignored */
    SIGUSR1   = 10,
    SIGSEGV   = 11,
    SIGUSR2   = 12,
    SIGPIPE   = 13,
    SIGALRM   = 14,
    SIGTERM   = 15,
    SIGCHLD   = 17,
    SIGCONT   = 18,
    SIGSTOP   = 19,  /* cannot be caught/ignored */
    SIGTSTP   = 20,
    SIGTTIN   = 21,
    SIGTTOU   = 22,
    SIGPOLL   = 29,
    SIGSYS    = 31,
    SIGRTMIN  = 34,
    SIGRTMAX  = 64
} signal_t;

#define SIG_MIN    1
#define SIG_MAX    SIGRTMAX
#define SIG_IS_REALTIME(s) ((s) >= SIGRTMIN && (s) <= SIGRTMAX)

/* sigset implementation: bitset sized to SIG_MAX */
typedef struct {
    unsigned long bits[((SIG_MAX + (sizeof(unsigned long)*8 - 1)) / (sizeof(unsigned long)*8))];
} sigset_t;

/* sigset helpers (inline for performance) */
static inline void sigemptyset(sigset_t *set)
{
    size_t n = sizeof(set->bits) / sizeof(set->bits[0]);
    for (size_t i = 0; i < n; ++i) set->bits[i] = 0UL;
}

static inline void sigfillset(sigset_t *set)
{
    size_t n = sizeof(set->bits) / sizeof(set->bits[0]);
    for (size_t i = 0; i < n; ++i) set->bits[i] = ~0UL;
}

static inline void sigaddset(sigset_t *set, int sig)
{
    if (sig < SIG_MIN || sig > SIG_MAX) return;
    unsigned idx = (sig - 1) / (sizeof(unsigned long)*8);
    unsigned off = (sig - 1) % (sizeof(unsigned long)*8);
    set->bits[idx] |= (1UL << off);
}

static inline void sigdelset(sigset_t *set, int sig)
{
    if (sig < SIG_MIN || sig > SIG_MAX) return;
    unsigned idx = (sig - 1) / (sizeof(unsigned long)*8);
    unsigned off = (sig - 1) % (sizeof(unsigned long)*8);
    set->bits[idx] &= ~(1UL << off);
}

static inline int sigismember(const sigset_t *set, int sig)
{
    if (sig < SIG_MIN || sig > SIG_MAX) return 0;
    unsigned idx = (sig - 1) / (sizeof(unsigned long)*8);
    unsigned off = (sig - 1) % (sizeof(unsigned long)*8);
    return (set->bits[idx] & (1UL << off)) != 0;
}

/* Minimal siginfo for kernel->user delivery (extend later) */
typedef struct {
    int si_signo;
    int si_errno;
    int si_code;
    void *si_addr;   /* faulting address for SEGV/BUS */
    pid_t si_pid;    /* sender pid for queued signals */
    int si_status;   /* exit status for SIGCHLD */
} siginfo_t;

/* sigaction skeleton (expand with flags and union as needed) */
typedef void (*sighandler_t)(int);
typedef void (*sighandler_siginfo_t)(int, siginfo_t *, void *);

struct sigaction {
    union {
        sighandler_t sa_handler;
        sighandler_siginfo_t sa_sigaction;
    };
    sigset_t sa_mask;
    unsigned sa_flags;
};

#endif /* _KERNEL_SIGNAL_H */
