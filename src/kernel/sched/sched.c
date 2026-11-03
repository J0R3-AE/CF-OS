#include "kernel/sched/sched.h"

#include "kernel/arch/io.h"
#include "kernel/arch/tss.h"
#include "kernel/mm/heap.h"
#include "kernel/mm/paging.h"
#include "kernel/proc/proc.h"

Thread *g_current;
Thread *g_idle_thread;
static Link run_queue;
static Thread *sleep_queue;
static int reschedule_pending;

Thread *sched_current(void)
{
    return g_current;
}

static void idle_thread_entry(void *arg)
{
    (void)arg;
    for (;;)
    {
        __asm__ volatile("sti; hlt");
    }
}

void runqueue_init(void)
{
    LinkInit(&run_queue);
}

void runqueue_add(Thread *thread)
{
    if (!thread || thread == g_idle_thread)
        return;
    if (thread->run_link.next &&
        thread->run_link.next != &thread->run_link)
        return;

    if (!thread->run_link.next)
        LinkInit(&thread->run_link);
    thread->state = THREAD_RUNNABLE;
    LinkBefore(&run_queue, &thread->run_link);
}

void runqueue_remove(Thread *thread)
{
    if (!thread || !thread->run_link.next ||
        thread->run_link.next == &thread->run_link)
        return;
    LinkRemove(&thread->run_link);
}

Thread *runqueue_next(Thread *from)
{
    (void)from;
    if (!ListIsEmpty(&run_queue))
    {
        Link *link = run_queue.next;
        Thread *thread = LinkData(link, Thread, run_link);
        LinkRemove(link);
        return thread;
    }
    return g_idle_thread;
}

void sched_activate_thread(Thread *thread)
{
    if (!thread)
        return;

    g_current = thread;
    thread->state = THREAD_RUNNING;

    process_t *process = thread->proc;
    u32 page_directory = process && process->pd_phys
                             ? process->pd_phys
                             : kernel_pd_phys;
    tss_set_kernel_stack((u32)(thread->stack + thread->stack_size));
    paging_switch_phys(page_directory);
}

void sched_init(void)
{
    g_current = NULL;
    g_idle_thread = NULL;
    g_pit_ticks = 0;
    sleep_queue = NULL;
    reschedule_pending = 0;
    runqueue_init();

    u8 *stack = malloc(4096);
    if (!stack)
    {
        KLOG_ERROR("sched: failed to allocate idle stack");
        return;
    }

    g_idle_thread = thread_create(idle_thread_entry, NULL, stack, 4096);
    if (!g_idle_thread)
    {
        free(stack);
        KLOG_ERROR("sched: failed to create idle thread");
    }
}

void sched_add(Thread *thread)
{
    runqueue_add(thread);
}

void sched_start(void)
{
    Thread *next = runqueue_next(NULL);
    if (!next)
    {
        KLOG_ERROR("sched: no idle thread");
        return;
    }

    sched_activate_thread(next);
    context_switch(NULL, &next->ctx);
    for (;;)
        halt();
}

void ksched_yield(void)
{
    Thread *current = g_current;
    if (current && current != g_idle_thread &&
        current->state == THREAD_RUNNING)
        runqueue_add(current);

    Thread *next = runqueue_next(current);
    if (!next || next == current)
    {
        if (current)
            current->state = THREAD_RUNNING;
        return;
    }

    sched_activate_thread(next);
    context_switch(current ? &current->ctx : NULL, &next->ctx);
}

void sched_yield(void)
{
    ksched_yield();
}

void sched_sleep(u64 ticks)
{
    Thread *current = g_current;
    if (!current || current == g_idle_thread)
        return;

    current->wakeup_tick = (u64)g_pit_ticks + (ticks ? ticks : 1);
    current->state = THREAD_SLEEPING;
    current->next_sleep = sleep_queue;
    sleep_queue = current;
    ksched_yield();
}

void sched_wake_sleepers(void)
{
    Thread **link = &sleep_queue;
    while (*link)
    {
        Thread *thread = *link;
        if (thread->wakeup_tick <= g_pit_ticks)
        {
            *link = thread->next_sleep;
            thread->next_sleep = NULL;
            thread->wakeup_tick = 0;
            runqueue_add(thread);
            continue;
        }
        link = &thread->next_sleep;
    }
}

void sched_tick(void)
{
    sched_wake_sleepers();
    reschedule_pending = 1;
}

void sched_maybe_resched(void)
{
    if (reschedule_pending)
    {
        reschedule_pending = 0;
        ksched_yield();
    }
}

u64 sched_ticks(void)
{
    return g_pit_ticks;
}
