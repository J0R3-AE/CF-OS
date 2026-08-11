/* kernel/signal/signal.c
 *
 * Minimal stubbed per-thread signal state and helper API.
 * All functions are TODO stubs that return errors or no-ops.
 *
 * Purpose:
 *   - Provide a stable API surface for the rest of the kernel while you
 *     implement full signal semantics later.
 *   - Each stub documents the intended behavior and integration points.
 *
 * Integration notes:
 *   - Replace the local thread_sigstate_t with the real thread_t member.
 *   - Implement locking if signals can be added from multiple CPUs.
 *   - Wire signal_add into fault handlers, kill syscall, timers, and child exit.
 *   - Wire signal_fetch into the scheduler or syscall return path to arrange delivery.
 */

#include "signal.h"
#include "libc/types.h"
#include "libc/errno.h"
#include "libc/string.h"

/* Forward-declare RT queue type and functions from queue.c
 * TODO: when queue.c is implemented, move rtq_t and prototypes to a header.
 */
typedef struct rtq_t rtq_t;
void rtq_init(rtq_t *q);
int rtq_is_empty(const rtq_t *q);
int rtq_is_full(const rtq_t *q);
int rtq_enqueue(rtq_t *q, int sig, const siginfo_t *info);
int rtq_dequeue(rtq_t *q, int *out_sig, siginfo_t *out_info);

/* Per-thread signal state placeholder.
 * TODO: embed this structure inside your real thread_t and remove the typedef here.
 */
typedef struct {
    sigset_t mask;    /* blocked signals */
    sigset_t pending; /* pending non-RT signals */
    rtq_t *rtq;       /* pointer to real-time queue instance */
} thread_sigstate_t;

/* Initialize a thread's signal state.
 * TODO:
 *  - If rtq is stored inline, initialize it here.
 *  - If rtq is allocated, allocate and initialize it here.
 *  - Zero the pending bitset and set default mask.
 */
void signal_thread_init(thread_sigstate_t *s)
{
    if (!s) return;
    sigemptyset(&s->mask);
    sigemptyset(&s->pending);
    s->rtq = NULL; /* TODO: allocate or point to embedded rtq */
}

/* Add a signal to a thread's pending set or RT queue.
 * Returns 0 on success, negative errno on failure.
 *
 * TODO:
 *  - Validate arguments and signal range.
 *  - If signal is real-time:
 *      - Ensure rtq exists, allocate if necessary.
 *      - Enqueue with rtq_enqueue and handle full queue.
 *  - Else:
 *      - Set the bit in s->pending.
 *  - If the target thread is sleeping or blocked in the scheduler, wake it.
 *  - If the signal is SIGKILL or SIGSTOP, handle kernel-only semantics.
 */
int signal_add(thread_sigstate_t *s, int sig, const siginfo_t *info)
{
    (void)s;
    (void)sig;
    (void)info;
    return -ENOSYS; /* not implemented yet */
}

/* Fetch the next deliverable signal for this thread.
 * On success set *out_sig and optionally fill out_info, return 0.
 * Return -ENOENT if none available, -EINVAL for bad args.
 *
 * TODO:
 *  - Prefer RT queue entries first.
 *  - Otherwise scan pending bitset for the lowest-numbered unmasked signal.
 *  - Clear the pending bit when returning a non-RT signal.
 *  - Respect per-thread mask in s->mask.
 *  - Consider performance: scanning every signal is simple but can be optimized.
 */
int signal_fetch(thread_sigstate_t *s, int *out_sig, siginfo_t *out_info)
{
    (void)s;
    (void)out_sig;
    (void)out_info;
    return -ENOENT; /* no signal available placeholder */
}

/* Block signals in 'set' by OR-ing into the thread mask.
 * Returns 0 on success, -EINVAL on bad args.
 *
 * TODO:
 *  - Implement atomic update of mask if concurrent access is possible.
 *  - Return previous mask if requested by syscall semantics.
 */
int signal_block(thread_sigstate_t *s, const sigset_t *set)
{
    if (!s || !set) return -EINVAL;
    /* TODO: atomic OR of mask bits */
    return -ENOSYS;
}

/* Unblock signals in 'set' by clearing bits from the thread mask.
 * Returns 0 on success, -EINVAL on bad args.
 *
 * TODO:
 *  - Implement atomic update of mask.
 *  - After unblocking, check for newly unmasked pending signals and arrange delivery.
 */
int signal_unblock(thread_sigstate_t *s, const sigset_t *set)
{
    if (!s || !set) return -EINVAL;
    /* TODO: atomic AND NOT of mask bits and trigger delivery if needed */
    return -ENOSYS;
}

/* Check if any unmasked pending signal exists for fast scheduler checks.
 * Returns 1 if there is at least one deliverable signal, 0 otherwise.
 *
 * TODO:
 *  - Check RT queue quickly.
 *  - Check pending bitset masked by s->mask.
 *  - This function should be cheap because scheduler may call it frequently.
 */
int signal_has_pending_unmasked(thread_sigstate_t *s)
{
    if (!s) return 0;
    /* TODO: implement efficient check */
    return 0;
}

/* Utility: clear all pending signals for a thread.
 * TODO: used when a thread exits or is reaped.
 */
void signal_clear_all(thread_sigstate_t *s)
{
    if (!s) return;
    sigemptyset(&s->pending);
    /* TODO: free or reset rtq if allocated */
}

/* TODOs and integration checklist:
 *
 * 1) Move thread_sigstate_t into your real thread_t structure and update callers.
 * 2) Implement a real rtq_t in queue.c and expose its header for inclusion here.
 * 3) Add locking or use per-thread guarantees to protect concurrent access.
 * 4) Implement syscalls:
 *      - sys_sigaction to set handlers
 *      - sys_sigprocmask to change masks
 *      - sys_kill to send signals between processes
 *      - sys_sigpending, sys_sigsuspend, sys_sigreturn
 * 5) Implement delivery:
 *      - When signal_fetch returns a signal, consult the process sigaction table
 *      - If handler is default, perform default action
 *      - If handler is custom, set up user stack and set RIP to trampoline
 * 6) Implement a secure user-space trampoline and a kernel-only sigreturn syscall
 * 7) Add tests: SIGUSR1 handler, blocking/unblocking, RT queue ordering, synchronous faults
 */
