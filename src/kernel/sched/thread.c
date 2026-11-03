#include "kernel/sched/sched.h"

#include "kernel/mm/heap.h"
#include "kernel/proc/proc.h"

extern void signal_thread_init(thread_sigstate_t *state);

Thread *thread_create(void (*entry)(void *), void *arg,
                      u8 *stack_mem, size_t stack_size)
{
    if (!entry || !stack_mem || !stack_size)
        return NULL;

    Thread *thread = calloc(1, sizeof(*thread));
    if (!thread)
        return NULL;

    LinkInit(&thread->run_link);
    thread->stack = stack_mem;
    thread->stack_size = stack_size;
    thread->state = THREAD_RUNNABLE;
    thread->entry = entry;
    thread->arg = arg;
    signal_thread_init(&thread->signals);

    if (context_create(&thread->ctx, entry, arg,
                       stack_mem + stack_size) != 0)
    {
        free(thread);
        return NULL;
    }

    return thread;
}

void thread_exit(void)
{
    Thread *thread = sched_current();
    if (thread && thread->proc)
        proc_mark_exit(0);
    else if (thread)
        thread->state = THREAD_ZOMBIE;

    ksched_yield();
    for (;;)
        __asm__ volatile("cli; hlt");
}

void thread_sleep(u32 milliseconds)
{
    sched_sleep((milliseconds + 9) / 10);
}

void wake_sleepers(void)
{
    sched_wake_sleepers();
}
