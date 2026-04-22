// thread.c
#include "sched/sched.h"
#include "libk/mem.h"
#include "arch/context.h"
#include "proc/proc.h" // 🔥 NEW

static Thread *sleep_queue = NULL;
volatile u32 g_ticks = 0;

Thread *thread_create(void (*entry)(void *), void *arg,
                      u8 *stack_mem, size_t stack_size)
{
    Thread *t = malloc(sizeof(Thread));
    if (!t)
        return NULL;

    t->stack = stack_mem;
    t->stack_size = stack_size;
    t->state = THREAD_RUNNABLE;

    t->entry = entry;
    t->arg = arg;

    t->proc = NULL; // 🔥 IMPORTANT (avoid garbage pointer)

    void *stack_top = stack_mem + stack_size;
    t->ctx = context_create(entry, arg, stack_top);

    ListInit(&t->run_link);

    return t;
}

void thread_exit(void)
{
    Thread *t = g_current;

    t->state = THREAD_ZOMBIE;

    // TODO: later -> notify process / wait()

    ksched_yield();

    // Should NEVER return
    for (;;)
    {
        __asm__ volatile("cli; hlt");
    }
}

void thread_sleep(u32 ms)
{
    Thread *t = g_current;

    t->wakeup_tick = g_ticks + ms;
    t->state = THREAD_SLEEPING;

    // 🔥 REQUIRED: remove from runqueue
    runqueue_remove(t);

    // add to sleep list
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
            t->state = THREAD_RUNNABLE;

            *pp = t->next_sleep;

            runqueue_add(t);
        }
        else
        {
            pp = &t->next_sleep;
        }
    }
}