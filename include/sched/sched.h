/**
 * @file sched.h
 * @brief Kernel scheduler interface and thread control structures.
 *
 * This header defines the core scheduler API, thread control block (TCB),
 * run queue management, sleep queue handling, and the timer‑driven
 * scheduling pipeline. The implementation is split across:
 *
 *  - thread.c     — thread creation and lifecycle
 *  - queue.c      — run queue operations
 *  - pipeline.c   — tick handler and rescheduling logic
 *  - sched.c      — scheduler initialization and dispatch loop
 */

#pragma once

#include "libk/types.h"
#include "libk/link.h"
#include "arch/context.h"

/* -------------------------------------------------------------------------- */
/* Thread State                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @enum thread_state_t
 * @brief Execution state of a thread within the scheduler.
 */
typedef enum {
    THREAD_RUNNABLE,   /**< Ready to run; present in the run queue. */
    THREAD_RUNNING,    /**< Currently executing on the CPU. */
    THREAD_ZOMBIE,     /**< Finished execution; awaiting cleanup. */
    THREAD_SLEEPING,   /**< Blocked until a wakeup tick. */
} thread_state_t;

/* -------------------------------------------------------------------------- */
/* Thread Control Block (TCB)                                                 */
/* -------------------------------------------------------------------------- */

/**
 * @struct Thread
 * @brief Kernel thread control block.
 *
 * Represents a schedulable execution context. Threads contain their own
 * saved CPU state, stack, and scheduling metadata. The scheduler manages
 * these structures through run queues and sleep queues.
 */
typedef struct Thread {
    Link run_link;
    context_t ctx;

    u8 *stack;
    size_t stack_size;

    thread_state_t state;

    u64 wakeup_tick;
    struct Thread *next_sleep;

    void (*entry)(void *);
    void *arg;

    struct process *proc;   // 🔥 ADD THIS LINE
} Thread;
/* -------------------------------------------------------------------------- */
/* Global Scheduler State                                                     */
/* -------------------------------------------------------------------------- */

/**
 * @brief Pointer to the currently running thread.
 *
 * Updated on every context switch.
 */
extern Thread *g_current;

/**
 * @brief Global tick counter incremented by sched_tick().
 */
extern volatile u32 g_ticks;

/* -------------------------------------------------------------------------- */
/* Thread Lifecycle (thread.c)                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Create a new kernel thread.
 *
 * @param entry      Entry function executed by the thread.
 * @param arg        Argument passed to the entry function.
 * @param stack_mem  Preallocated stack memory.
 * @param stack_size Size of the provided stack.
 *
 * @return Pointer to the created thread, or NULL on failure.
 */
Thread *thread_create(void (*entry)(void *),
                      void *arg,
                      u8 *stack_mem,
                      size_t stack_size);

/* -------------------------------------------------------------------------- */
/* Run Queue Operations (queue.c)                                             */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the run queue subsystem.
 */
void runqueue_init(void);

/**
 * @brief Add a thread to the run queue.
 *
 * @param t Thread to enqueue.
 */
void runqueue_add(Thread *t);

/**
 * @brief Select the next runnable thread.
 *
 * @param from The thread currently running (may be NULL).
 * @return Pointer to the next thread to run.
 */
Thread *runqueue_next(Thread *from);

/* -------------------------------------------------------------------------- */
/* Scheduling Pipeline (pipeline.c)                                           */
/* -------------------------------------------------------------------------- */

/**
 * @brief Timer tick handler invoked from the PIT ISR.
 *
 * Updates tick count, wakes sleeping threads, and may trigger a reschedule.
 */
void sched_tick(void);

/**
 * @brief Check whether a reschedule is needed and perform it if so.
 *
 * Called at safe points where preemption is allowed.
 */
void sched_maybe_resched(void);

/**
 * @brief Voluntarily yield the CPU to another runnable thread.
 */
void ksched_yield(void);

/* -------------------------------------------------------------------------- */
/* Scheduler Core (sched.c)                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Initialize the scheduler subsystem.
 */
void sched_init(void);

/**
 * @brief Add a thread to the scheduler.
 *
 * @param t Thread to register.
 */
void sched_add(Thread *t);

/**
 * @brief Start the scheduler and begin executing threads.
 *
 * This function does not return.
 */
void sched_start(void);

/* -------------------------------------------------------------------------- */
/* Sleep Queue                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @brief Put the current thread to sleep for a number of milliseconds.
 *
 * @param ms Duration to sleep.
 */
void thread_sleep(u32 ms);

/**
 * @brief Wake any sleeping threads whose wakeup time has passed.
 */
void wake_sleepers(void);

