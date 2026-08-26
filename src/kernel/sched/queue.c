#include "kernel/sched/queue.h"
#include "kernel/sched/thread.h"

/* -------------------------------------------------------------------------- */
/* Initialize                                                                 */
/* -------------------------------------------------------------------------- */

void thread_queue_init(thread_queue_t *queue)
{
    if (!queue)
        return;

    queue->head = NULL;
    queue->tail = NULL;
}


/* -------------------------------------------------------------------------- */
/* Push                                                                       */
/* -------------------------------------------------------------------------- */

void thread_queue_push(
    thread_queue_t *queue,
    thread_t *thread)
{
    if (!queue || !thread || thread->queued)
        return;

    thread->next = NULL;
    thread->queued = true;

    if (!queue->head)
    {
        queue->head = thread;
        queue->tail = thread;
        return;
    }

    queue->tail->next = thread;
    queue->tail = thread;
}


/* -------------------------------------------------------------------------- */
/* Remove                                                                     */
/* -------------------------------------------------------------------------- */

void thread_queue_remove(
    thread_queue_t *queue,
    thread_t *thread)
{
    if (!queue || !thread || !queue->head)
        return;

    thread_t *prev = NULL;
    thread_t *cur = queue->head;

    while (cur)
    {
        if (cur == thread)
{
    if (prev)
        prev->next = cur->next;
    else
        queue->head = cur->next;

    if (queue->tail == cur)
        queue->tail = prev;

    cur->next = NULL;
    cur->queued = false;

    return;
}

        prev = cur;
        cur = cur->next;
    }
}


/* -------------------------------------------------------------------------- */
/* Pop                                                                        */
/* -------------------------------------------------------------------------- */

thread_t *thread_queue_pop(
    thread_queue_t *queue)
{
    if (!queue || !queue->head)
        return NULL;

    thread_t *thread = queue->head;

    queue->head = thread->next;

    if (!queue->head)
        queue->tail = NULL;

    thread->next = NULL;
    thread->queued = false;

    return thread;
}


/* -------------------------------------------------------------------------- */
/* Front                                                                      */
/* -------------------------------------------------------------------------- */

thread_t *thread_queue_front(
    const thread_queue_t *queue)
{
    if (!queue)
        return NULL;

    return queue->head;
}


/* -------------------------------------------------------------------------- */
/* Empty                                                                      */
/* -------------------------------------------------------------------------- */

int thread_queue_empty(
    const thread_queue_t *queue)
{
    if (!queue)
        return 1;

    return queue->head == NULL;
}