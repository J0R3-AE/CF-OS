#pragma once

#include "libk/types.h"
#include "libk/link.h"
#include "arch/context.h"
#include "libk/log.h"

struct process;

typedef enum
{
    THREAD_RUNNABLE,
    THREAD_RUNNING,
    THREAD_ZOMBIE,
    THREAD_SLEEPING,
} thread_state_t;

typedef struct Thread
{
    Link run_link;
    context_t ctx;

    u8 *stack;
    size_t stack_size;

    thread_state_t state;

    u64 wakeup_tick;
    struct Thread *next_sleep;

    void (*entry)(void *);
    void *arg;

    struct process *proc;
} Thread;

extern Thread *g_current;
extern Thread *g_idle_thread;
extern volatile u32 g_ticks;

Thread *thread_create(void (*entry)(void *),
                      void *arg,
                      u8 *stack_mem,
                      size_t stack_size);

void thread_exit(void);
void thread_sleep(u32 ms);
void wake_sleepers(void);

void runqueue_init(void);
void runqueue_add(Thread *t);
void runqueue_remove(Thread *t);
Thread *runqueue_next(Thread *from);

void sched_tick(void);
void sched_maybe_resched(void);
void ksched_yield(void);

void sched_init(void);
void sched_add(Thread *t);
void sched_start(void);