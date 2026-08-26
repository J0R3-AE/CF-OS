#ifndef KERNEL_SIGNAL_H
#define KERNEL_SIGNAL_H

#include "libc/types.h"
#include "libc/link.h"

/* -------------------------------------------------------------------------- */
/* Signal numbers                                                             */
/* -------------------------------------------------------------------------- */

typedef enum
{
    SIGHUP    = 1,
    SIGINT    = 2,
    SIGQUIT   = 3,
    SIGILL    = 4,
    SIGTRAP   = 5,
    SIGABRT   = 6,
    SIGBUS    = 7,
    SIGFPE    = 8,
    SIGKILL   = 9,
    SIGUSR1   = 10,
    SIGSEGV   = 11,
    SIGUSR2   = 12,
    SIGPIPE   = 13,
    SIGALRM   = 14,
    SIGTERM   = 15,
    SIGCHLD   = 17,
    SIGCONT   = 18,
    SIGSTOP   = 19,
    SIGTSTP   = 20,
    SIGTTIN   = 21,
    SIGTTOU   = 22,
    SIGPOLL   = 29,
    SIGSYS    = 31,
    SIGRTMIN  = 34,
    SIGRTMAX  = 64
} signal_t;

#define SIG_MIN 1
#define SIG_MAX SIGRTMAX

#define SIG_IS_REALTIME(sig) \
    ((sig) >= SIGRTMIN && (sig) <= SIGRTMAX)


/* -------------------------------------------------------------------------- */
/* Signal handlers                                                            */
/* -------------------------------------------------------------------------- */

typedef struct siginfo siginfo_t;

typedef void (*sighandler_t)(int);

typedef void (*sighandler_siginfo_t)(
    int,
    siginfo_t *,
    void *
);

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)


/* -------------------------------------------------------------------------- */
/* Signal set                                                                 */
/* -------------------------------------------------------------------------- */

typedef struct sigset
{
    unsigned long bits[
        (SIG_MAX + (sizeof(unsigned long) * 8 - 1)) /
        (sizeof(unsigned long) * 8)
    ];

} sigset_t;


static inline void sigemptyset(sigset_t *set)
{
    if (!set)
        return;

    usize n =
        sizeof(set->bits) /
        sizeof(set->bits[0]);

    for (usize i = 0; i < n; ++i)
        set->bits[i] = 0UL;
}


static inline void sigfillset(sigset_t *set)
{
    if (!set)
        return;

    usize n =
        sizeof(set->bits) /
        sizeof(set->bits[0]);

    for (usize i = 0; i < n; ++i)
        set->bits[i] = ~0UL;
}


static inline void sigaddset(
    sigset_t *set,
    int sig)
{
    if (!set ||
        sig < SIG_MIN ||
        sig > SIG_MAX)
        return;

    unsigned bits =
        sizeof(unsigned long) * 8;

    unsigned idx =
        (unsigned)(sig - 1) / bits;

    unsigned off =
        (unsigned)(sig - 1) % bits;

    set->bits[idx] |=
        (1UL << off);
}


static inline void sigdelset(
    sigset_t *set,
    int sig)
{
    if (!set ||
        sig < SIG_MIN ||
        sig > SIG_MAX)
        return;

    unsigned bits =
        sizeof(unsigned long) * 8;

    unsigned idx =
        (unsigned)(sig - 1) / bits;

    unsigned off =
        (unsigned)(sig - 1) % bits;

    set->bits[idx] &=
        ~(1UL << off);
}


static inline int sigismember(
    const sigset_t *set,
    int sig)
{
    if (!set ||
        sig < SIG_MIN ||
        sig > SIG_MAX)
        return 0;

    unsigned bits =
        sizeof(unsigned long) * 8;

    unsigned idx =
        (unsigned)(sig - 1) / bits;

    unsigned off =
        (unsigned)(sig - 1) % bits;

    return
        (set->bits[idx] &
        (1UL << off)) != 0;
}


/* -------------------------------------------------------------------------- */
/* Signal information                                                         */
/* -------------------------------------------------------------------------- */

struct siginfo
{
    int si_signo;
    int si_errno;
    int si_code;

    void *si_addr;

    int si_status;
};


/* -------------------------------------------------------------------------- */
/* Signal action                                                              */
/* -------------------------------------------------------------------------- */

typedef struct sigaction
{
    union
    {
        sighandler_t sa_handler;
        sighandler_siginfo_t sa_sigaction;
    };

    sigset_t sa_mask;
    unsigned sa_flags;

} sigaction_t;


/* -------------------------------------------------------------------------- */
/* Realtime signal queue                                                      */
/* -------------------------------------------------------------------------- */

#ifndef RTQ_CAPACITY
#define RTQ_CAPACITY 64
#endif

typedef struct rtq_entry
{
    Link link;

    int sig;
    siginfo_t info;

} rtq_entry_t;


typedef struct rtq
{
    Link used_head;
    Link free_head;

    rtq_entry_t pool[RTQ_CAPACITY];

    unsigned count;

} rtq_t;


/* -------------------------------------------------------------------------- */
/* Per-thread signal state                                                    */
/* -------------------------------------------------------------------------- */

typedef struct thread_sigstate
{
    sigset_t mask;
    sigset_t pending;

    rtq_t rt_queue;

} thread_sigstate_t;


/* -------------------------------------------------------------------------- */
/* Realtime queue API                                                         */
/* -------------------------------------------------------------------------- */

void rtq_init(rtq_t *q);

int rtq_is_empty(
    const rtq_t *q);

int rtq_is_full(
    const rtq_t *q);

int rtq_enqueue(
    rtq_t *q,
    int sig,
    const siginfo_t *info);

int rtq_dequeue(
    rtq_t *q,
    int *out_sig,
    siginfo_t *out_info);

int rtq_peek(
    const rtq_t *q,
    int *out_sig,
    siginfo_t *out_info);

unsigned rtq_count(
    const rtq_t *q);


/* -------------------------------------------------------------------------- */
/* Thread signal API                                                          */
/* -------------------------------------------------------------------------- */

void signal_thread_init(
    thread_sigstate_t *s);

int signal_add(
    thread_sigstate_t *s,
    int sig,
    const siginfo_t *info);

int signal_fetch(
    thread_sigstate_t *s,
    int *out_sig,
    siginfo_t *out_info);

int signal_block(
    thread_sigstate_t *s,
    const sigset_t *set);

int signal_unblock(
    thread_sigstate_t *s,
    const sigset_t *set);

int signal_has_pending_unmasked(
    thread_sigstate_t *s);

void signal_clear_all(
    thread_sigstate_t *s);

#endif /* KERNEL_SIGNAL_H */