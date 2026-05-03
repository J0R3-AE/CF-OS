#include "sched/sched.h"

#include "libk/mem.h"
#include "arch/context.h"
#include "mm/heap.h"

static Thread *sleep_queue = NULL;
volatile u32 g_ticks = 0;

Thread *thread_create(void (*entry)(void *), void *arg,
                      u8 *stack_mem, size_t stack_size)
{
    Thread *t = malloc(sizeof(Thread));
    if (!t)
        return NULL;

    memset(t, 0, sizeof(*t));

    t->stack = stack_mem;
    t->stack_size = stack_size;
    t->state = THREAD_RUNNABLE;
    t->entry = entry;
    t->arg = arg;
    t->proc = NULL;

    void *stack_top = stack_mem + stack_size;
    t->ctx = context_create(entry, arg, stack_top);

    ListInit(&t->run_link);
    t->next_sleep = NULL;

    return t;
}

void thread_exit(void)
{
    if (g_current)
        g_current->state = THREAD_ZOMBIE;

    ksched_yield();

    for (;;)
        __asm__ volatile("cli; hlt");
}

void thread_sleep(u32 ms)
{
    Thread *t = g_current;
    if (!t)
        return;

    t->wakeup_tick = g_ticks + ms;
    t->state = THREAD_SLEEPING;

    runqueue_remove(t);

    t->next_sleep = sleep_queue;
    sleep_queue = t;

    ksched_yield();
}

void wake_sleepers(void)
{
    Thread **pp = &sleep_queue;

    while (*pp)
    {
        Thread *t = *pp;

        if (t->wakeup_tick <= g_ticks)
        {
            *pp = t->next_sleep;
            t->next_sleep = NULL;
            t->state = THREAD_RUNNABLE;
            runqueue_add(t);
        }
        else
        {
            pp = &t->next_sleep;
        }
    }
}