#include "kernel/sched/thread.h"
#include "kernel/sched/scheduler.h"
#include "kernel/sched/context.h"
#include "kernel/signal/signal.h"

#include "kernel/mm/heap.h"
#include "libc/log.h"

static u32 next_tid = 1;

extern void signal_thread_init(thread_sigstate_t *s);

static void thread_trampoline(void *arg)
{
    thread_t *thread = (thread_t *)arg;

    if (!thread || !thread->entry)
    {
        thread_exit();
        return;
    }

    thread->state = THREAD_RUNNING;

    thread->entry(thread->arg);

    thread_exit();
}

thread_t *thread_create(
    void (*entry)(void *),
    void *arg,
    usize stack_size)
{
    if (!entry || stack_size == 0)
        return NULL;

    thread_t *thread =
        calloc(1, sizeof(thread_t));

    if (!thread)
        return NULL;

    thread->stack = malloc(stack_size);

    if (!thread->stack)
    {
        free(thread);
        return NULL;
    }

    thread->tid = next_tid++;
    thread->state = THREAD_READY;

    thread->stack_size = stack_size;

    thread->entry = entry;
    thread->arg = arg;

    signal_thread_init(&thread->signals);

    thread->process = NULL;
    thread->wakeup_tick = 0;
    thread->next = NULL;

    u8 *stack_top =
        (u8 *)thread->stack + stack_size;

    if (context_create(
            &thread->context,
            thread_trampoline,
            thread,
            stack_top) < 0)
    {
        free(thread->stack);
        free(thread);
        return NULL;
    }

    return thread;
}

void thread_destroy(thread_t *thread)
{
    if (!thread)
        return;

    if (thread->stack)
        free(thread->stack);

    free(thread);
}

void thread_exit(void)
{
    thread_t *thread = sched_current();

    if (!thread)
        return;

    thread->state = THREAD_ZOMBIE;

    sched_yield();

    for (;;)
        asm volatile("hlt");
}