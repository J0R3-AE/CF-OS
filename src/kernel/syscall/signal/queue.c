/* kernel/signal/queue.c
 *
 * Real-time signal queue implemented with the intrusive Link list (link.h).
 *
 * - Fixed capacity pool of entries (RTQ_CAPACITY).
 * - FIFO semantics: dequeue returns the oldest enqueued entry.
 * - Uses LinkInit/LinkAfter/LinkBefore/LinkRemove and LinkData macros from link.h.
 *
 * Notes / TODO:
 *  - This implementation is NOT thread-safe. Add a spinlock or other
 *    synchronization if signals can be enqueued from multiple CPUs/contexts.
 *  - Consider making capacity configurable or using dynamic allocation.
 *  - Consider exposing a header (queue.h) with rtq_t and function prototypes.
 */

#include <signal.h>
#include <libc/link.h>
#include <libc/types.h>
#include <libc/errno.h>
#include <libc/string.h>

/* Tunable capacity for the RT queue */
#ifndef RTQ_CAPACITY
#define RTQ_CAPACITY 64
#endif

/* One queue entry: intrusive Link plus payload (sig and siginfo) */
typedef struct rtq_entry {
    Link link;         /* intrusive list node */
    int sig;           /* signal number */
    siginfo_t info;    /* associated siginfo (copied) */
} rtq_entry_t;

/* Real-time queue structure */
typedef struct {
    Link used_head;                /* head for queued (used) entries; FIFO: pop from next */
    Link free_head;                /* head for free entries */
    rtq_entry_t pool[RTQ_CAPACITY];/* fixed pool of entries */
    unsigned count;                /* number of used entries */
} rtq_t;

/* Initialize the queue: set up free list with all pool entries and empty used list */
void rtq_init(rtq_t *q)
{
    if (!q) return;

    /* init list heads */
    LinkInit(&q->used_head);
    LinkInit(&q->free_head);

    /* place all pool entries onto the free list */
    for (unsigned i = 0; i < RTQ_CAPACITY; ++i) {
        rtq_entry_t *e = &q->pool[i];
        /* clear payload */
        e->sig = 0;
        memset(&e->info, 0, sizeof(e->info));
        /* init link and insert at free_head (any order is fine) */
        LinkInit(&e->link);
        /* insert after free_head (push front) */
        LinkAfter(&q->free_head, &e->link);
    }

    q->count = 0;
}

/* Return non-zero if queue is empty */
int rtq_is_empty(const rtq_t *q)
{
    if (!q) return 1;
    return ListIsEmpty(&q->used_head);
}

/* Return non-zero if queue is full */
int rtq_is_full(const rtq_t *q)
{
    if (!q) return 0;
    /* full when no free entries remain */
    return ListIsEmpty(&q->free_head) ? 1 : 0;
}

/* Enqueue a real-time signal.
 * - sig: signal number
 * - info: optional siginfo pointer (copied if non-NULL)
 *
 * Returns:
 *   0      on success
 *  -EAGAIN if queue is full
 *  -EINVAL if args invalid
 *
 * TODO:
 *  - Add locking if concurrent producers/consumers exist.
 *  - Optionally wake a sleeping thread after enqueue.
 */
int rtq_enqueue(rtq_t *q, int sig, const siginfo_t *info)
{
    if (!q) return -EINVAL;
    if (sig < SIG_MIN || sig > SIG_MAX) return -EINVAL;

    /* If no free entries, queue is full */
    if (rtq_is_full(q)) return -EAGAIN;

    /* Pop one entry from free_head (free list head->next) */
    Link *free_node = q->free_head.next;
    if (!free_node || free_node == &q->free_head) {
        /* unexpected empty free list */
        return -EAGAIN;
    }

    /* Detach from free list */
    LinkRemove(free_node);

    /* Fill payload */
    rtq_entry_t *entry = LinkData(free_node, rtq_entry_t, link);
    entry->sig = sig;
    if (info)
        entry->info = *info;
    else {
        memset(&entry->info, 0, sizeof(entry->info));
        entry->info.si_signo = sig;
    }

    /* Insert at tail of used list to preserve FIFO.
     * used_head.prev is the current tail; insert before head to append.
     */
    LinkBefore(&q->used_head, &entry->link);

    q->count++;

    /* TODO: memory barriers or wakeup logic if needed */

    return 0;
}

/* Dequeue the oldest real-time signal.
 * - out_sig/out_info may be NULL if caller only wants to drop the entry.
 *
 * Returns:
 *   0      on success (out_sig/out_info filled if non-NULL)
 *  -ENOENT if queue empty
 *  -EINVAL if args invalid
 *
 * TODO:
 *  - Add locking if concurrent producers/consumers exist.
 */
int rtq_dequeue(rtq_t *q, int *out_sig, siginfo_t *out_info)
{
    if (!q) return -EINVAL;

    if (rtq_is_empty(q)) return -ENOENT;

    /* Oldest entry is used_head.next */
    Link *node = q->used_head.next;
    if (!node || node == &q->used_head) return -ENOENT;

    /* Detach from used list */
    LinkRemove(node);

    /* Extract payload */
    rtq_entry_t *entry = LinkData(node, rtq_entry_t, link);
    if (out_sig) *out_sig = entry->sig;
    if (out_info) *out_info = entry->info;

    /* Clear payload (optional) */
    entry->sig = 0;
    memset(&entry->info, 0, sizeof(entry->info));

    /* Return entry to free list (push front) */
    LinkAfter(&q->free_head, &entry->link);

    if (q->count > 0) q->count--;

    /* TODO: memory barriers or wakeup logic if needed */

    return 0;
}

/* Optional helper: peek at oldest entry without removing it.
 * Returns 0 on success, -ENOENT if empty.
 */
int rtq_peek(const rtq_t *q, int *out_sig, siginfo_t *out_info)
{
    if (!q) return -EINVAL;
    if (rtq_is_empty(q)) return -ENOENT;

    Link *node = q->used_head.next;
    if (!node || node == &q->used_head) return -ENOENT;

    rtq_entry_t *entry = LinkData(node, rtq_entry_t, link);
    if (out_sig) *out_sig = entry->sig;
    if (out_info) *out_info = entry->info;
    return 0;
}

/* Optional helper: return number of queued entries (cheap, maintained on enqueue/dequeue) */
unsigned rtq_count(const rtq_t *q)
{
    if (!q) return 0;
    return q->count;
}

/* End of queue implementation */
