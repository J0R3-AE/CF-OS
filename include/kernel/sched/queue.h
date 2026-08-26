#pragma once

struct thread;

typedef struct
{
    struct thread *head;
    struct thread *tail;
} thread_queue_t;


void thread_queue_init(thread_queue_t *queue);

void thread_queue_push(
    thread_queue_t *queue,
    struct thread *thread);

void thread_queue_remove(
    thread_queue_t *queue,
    struct thread *thread);

struct thread *thread_queue_pop(
    thread_queue_t *queue);

struct thread *thread_queue_front(
    const thread_queue_t *queue);

int thread_queue_empty(
    const thread_queue_t *queue);