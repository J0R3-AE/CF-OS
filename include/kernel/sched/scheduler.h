#pragma once

#include "libc/types.h"

struct thread;
typedef struct thread thread_t;

void sched_init(void);
void sched_add(thread_t *thread);
void sched_start(void);
void sched_yield(void);
void sched_tick(void);
void sched_sleep(u64 ticks);
void sched_wake_sleepers(void);
thread_t *sched_current(void);
thread_t *sched_idle(void);
u64 sched_ticks(void);