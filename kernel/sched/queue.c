#include "sched/sched.h"

Link g_run_queue;

extern Thread *g_idle_thread;

void runqueue_init(void)
{
    ListInit(&g_run_queue);
}

void runqueue_add(Thread *t)
{
    if (!t)
        return;

    if (t->run_link.next || t->run_link.prev)
        return;

    ListBefore(&g_run_queue, &t->run_link);
}

void runqueue_remove(Thread *t)
{
    if (!t)
        return;

    if (!t->run_link.next || !t->run_link.prev)
        return;

    ListRemove(&t->run_link);
    ListInit(&t->run_link);
}

Thread *runqueue_next(Thread *from)
{
    if (ListIsEmpty(&g_run_queue))
        return g_idle_thread;

    Link *start = from ? &from->run_link : &g_run_queue;
    Link *it = start->next;

    while (it != &g_run_queue)
    {
        Thread *t = LinkData(it, Thread, run_link);
        if (t->state == THREAD_RUNNABLE && t != g_idle_thread)
            return t;
        it = it->next;
    }

    it = g_run_queue.next;
    while (it != &g_run_queue)
    {
        Thread *t = LinkData(it, Thread, run_link);
        if (t->state == THREAD_RUNNABLE && t != g_idle_thread)
            return t;
        it = it->next;
    }

    return g_idle_thread;
}