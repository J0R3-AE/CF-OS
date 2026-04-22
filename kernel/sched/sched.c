// kernel/sched/sched.c
#include "sched/sched.h"
#include "libk/mem.h"

Thread *g_current = NULL;

static void idle_thread(void *arg)
{
    (void)arg;
    for (;;)
        __asm__ volatile("hlt");
}

void sched_init(void)
{
    runqueue_init();
    g_current = NULL;
}

void sched_add(Thread *t)
{
    if (!t)
        return;

    runqueue_add(t);
}

void sched_start(void)
{
    Thread *next = runqueue_next(NULL);
    if (!next)
        return;

    g_current = next;
    next->state = THREAD_RUNNING;

    context_switch(NULL, next->ctx);

    for (;;)
        __asm__ volatile("hlt");
}