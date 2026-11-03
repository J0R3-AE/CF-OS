#pragma once

#include "kernel/sched/sched.h"

typedef Thread thread_t;

void sched_yield(void);
void sched_sleep(u64 ticks);
void sched_wake_sleepers(void);
u64 sched_ticks(void);
