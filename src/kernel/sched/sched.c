#include "kernel/sched/scheduler.h"

#include "kernel/sched/thread.h"
#include "kernel/sched/queue.h"
#include "kernel/sched/context.h"

#include "libc/types.h"
#include "libc/log.h"
#include "kernel/mm/heap.h"

#include "kernel/arch/io.h"

/* -------------------------------------------------------------------------- */
/* kernel/scheduler state                                                            */
/* -------------------------------------------------------------------------- */

static thread_t *g_current = NULL;
static thread_t *g_idle_thread = NULL;
static thread_t *g_sleeping_threads = NULL;

static thread_queue_t g_ready_queue;

static u64 g_ticks = 0;


/* -------------------------------------------------------------------------- */
/* Idle thread                                                                */
/* -------------------------------------------------------------------------- */

static void idle_thread_entry(void *arg)
{
    (void)arg;

    for (;;)
    {
        /*
         * Interrupts must be enabled so the timer/keyboard/etc.
         * can wake the processor.
         */
        sti();
        halt();
    }
}


/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

void sched_init(void)
{
    thread_queue_init(&g_ready_queue);

    g_current = NULL;
    g_idle_thread = NULL;
    g_ticks = 0;

    /*
     * Create the idle thread.
     *
     * The kernel/scheduler itself owns this thread.
     */
    g_idle_thread = thread_create(
        idle_thread_entry,
        NULL,
        4096);

    if (!g_idle_thread)
    {
        KLOG_ERROR("sched: failed to create idle thread");
        return;
    }

    /*
     * Idle thread is not placed in the normal ready queue.
     * It is selected only when there are no runnable threads.
     */
    g_idle_thread->state = THREAD_READY;
}


/* -------------------------------------------------------------------------- */
/* Add thread                                                                 */
/* -------------------------------------------------------------------------- */

void sched_add(thread_t *thread)
{
    if (!thread)
        return;

    if (thread->state == THREAD_ZOMBIE)
        return;

    thread->state = THREAD_READY;

    thread_queue_push(
        &g_ready_queue,
        thread);
}


/* -------------------------------------------------------------------------- */
/* Pick next runnable thread                                                  */
/* -------------------------------------------------------------------------- */

static thread_t *sched_pick_next(void)
{
    thread_t *next = thread_queue_pop(&g_ready_queue);

    if (next)
        return next;

    return g_idle_thread;
}

static void sched_switch_to(thread_t *next)
{
    thread_t *current = g_current;

    if (!next)
        return;

    if (next == current)
        return;

    next->state = THREAD_RUNNING;
    g_current = next;

    context_switch(
        current ? &current->context : NULL,
        &next->context);
}

/* -------------------------------------------------------------------------- */
/* Start scheduler                                                            */
/* -------------------------------------------------------------------------- */

void sched_start(void)
{
    thread_t *next = sched_pick_next();

    if (!next)
    {
        KLOG_ERROR("sched: no idle thread");
        return;
    }

    g_current = next;
    next->state = THREAD_RUNNING;

    /*
     * No current context exists yet.
     *
     * context_switch() expects an old context pointer, so there is
     * nothing to save on the very first dispatch.
     */
    context_switch(
        NULL,
        &next->context);

    /*
     * We should never return here.
     */
    for (;;)
    {
        halt();
    }
}


/* -------------------------------------------------------------------------- */
/* Yield                                                                      */
/* -------------------------------------------------------------------------- */

void sched_yield(void)
{
    thread_t *current = g_current;

    if (!current)
        return;

    if (current != g_idle_thread &&
        current->state == THREAD_RUNNING)
    {
        current->state = THREAD_READY;
        thread_queue_push(&g_ready_queue, current);
    }

    thread_t *next = sched_pick_next();

    sched_switch_to(next);
}

void sched_sleep(u64 ticks)
{
    thread_t *current = g_current;

    if (!current || current == g_idle_thread)
        return;

    current->state = THREAD_SLEEPING;
    current->wakeup_tick = g_ticks + ticks;

    current->next = g_sleeping_threads;
    g_sleeping_threads = current;

    thread_t *next = sched_pick_next();

    sched_switch_to(next);
}

void sched_wake_sleepers(void)
{
    thread_t **link = &g_sleeping_threads;

    while (*link)
    {
        thread_t *thread = *link;

        if (thread->wakeup_tick <= g_ticks)
        {
            *link = thread->next;

            thread->next = NULL;
            thread->wakeup_tick = 0;
            thread->state = THREAD_READY;

            thread_queue_push(
                &g_ready_queue,
                thread);

            continue;
        }

        link = &thread->next;
    }
}

/* -------------------------------------------------------------------------- */
/* Timer tick                                                                 */
/* -------------------------------------------------------------------------- */

void sched_tick(void)
{
    g_ticks++;

    sched_wake_sleepers();
    /*
     * For the first kernel/scheduler version, use a simple round-robin
     * kernel/scheduler. Every timer tick gives another runnable thread
     * an opportunity to run.
     */

}


/* -------------------------------------------------------------------------- */
/* Accessors                                                                  */
/* -------------------------------------------------------------------------- */

thread_t *sched_current(void)
{
    return g_current;
}


thread_t *sched_idle(void)
{
    return g_idle_thread;
}


u64 sched_ticks(void)
{
    return g_ticks;
}