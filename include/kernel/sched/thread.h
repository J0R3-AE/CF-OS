#pragma once

#include "libc/types.h"
#include "kernel/signal/signal.h"
#include "context.h"

struct process;

typedef enum
{
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEPING,
    THREAD_BLOCKED,
    THREAD_ZOMBIE
} thread_state_t;

typedef struct thread
{
    u32 tid;
    thread_state_t state;

    context_t context;

    void *stack;
    usize stack_size;

    u64 wakeup_tick;

    struct process *process;

    void (*entry)(void *);
    void *arg;

    bool queued;
    struct thread *next;

    thread_sigstate_t signals;

} thread_t;


thread_t *thread_create(
    void (*entry)(void *),
    void *arg,
    usize stack_size);

void thread_destroy(thread_t *thread);

void thread_exit(void);