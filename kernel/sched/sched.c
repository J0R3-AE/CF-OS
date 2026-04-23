#include "sched/sched.h"
#include "libk/mem.h"

Thread *g_current = NULL;
Thread *g_idle_thread = NULL;

static void idle_thread(void *arg)
{
    (void)arg;

    for (;;)
        __asm__ volatile("sti; hlt");
}

void sched_init(void)
{
    runqueue_init();
    g_current = NULL;
    g_idle_thread = NULL;

    u8 *idle_stack = malloc(4096);
    if (!idle_stack)
        return;

    g_idle_thread = thread_create(idle_thread, NULL, idle_stack, 4096);
    if (g_idle_thread)
    {
        g_idle_thread->state = THREAD_RUNNABLE;
        ListInit(&g_idle_thread->run_link);
    }
}

void sched_add(Thread *t)
{
    if (!t)
        return;

    t->state = THREAD_RUNNABLE;
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