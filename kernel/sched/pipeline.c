#include "sched/sched.h"

static volatile int need_resched = 0;

void sched_tick(void)
{
    g_ticks++;
    wake_sleepers();
    need_resched = 1;
}

void sched_maybe_resched(void)
{
    if (need_resched)
    {
        need_resched = 0;
        ksched_yield();
    }
}

void ksched_yield(void)
{
    Thread *prev = g_current;
    Thread *next = runqueue_next(prev);

    if (!next || next == prev)
        return;

    if (prev && prev->state == THREAD_RUNNING)
        prev->state = THREAD_RUNNABLE;

    g_current = next;
    next->state = THREAD_RUNNING;

    if (prev)
        context_switch(&prev->ctx, next->ctx);
    else
        context_switch(NULL, next->ctx);
}