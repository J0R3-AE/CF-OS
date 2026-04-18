/**
 * @file context.h
 * @brief CPU context management for x86 software multitasking.
 *
 * This header defines the opaque context type used by the kernel’s
 * context‑switching mechanism. A context represents the saved CPU state
 * (typically the stack pointer) for a thread or process. The actual
 * register layout is defined by the assembly implementation of
 * `i386context_switch()`.
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "libk/types.h"

/* -------------------------------------------------------------------------- */
/* Context Type                                                                */
/* -------------------------------------------------------------------------- */

/**
 * @typedef context_t
 * @brief Opaque pointer to a saved CPU context.
 *
 * A context is represented as a pointer to a saved stack frame. The
 * exact layout depends on the assembly implementation of the context
 * switch routine.
 */
typedef u32 *context_t;

typedef struct context_ctx {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;

    u32 esi;
    u32 edi;
    u32 ebp;

    u32 eip;
    u32 esp;
    u32 eflags;

    u32 cs;
    u32 ds;
    u32 es;
    u32 fs;
    u32 gs;

    u32 cr3;
} context_ctx_t;

/* -------------------------------------------------------------------------- */
/* External Assembly Routine                                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief Low‑level context switch implemented in assembly.
 *
 * Saves the current CPU state into `old` and loads the new state from
 * `new`. This function performs the actual register restoration and
 * stack switching.
 *
 * @param old Pointer to store the outgoing context (ESP).
 * @param new Pointer to the incoming context (ESP).
 */
extern void i386context_switch(context_t *old, context_t new);

/* -------------------------------------------------------------------------- */
/* High‑Level Context API                                                      */
/* -------------------------------------------------------------------------- */

/**
 * @brief Create a new execution context.
 *
 * Allocates and initializes a CPU context that will begin execution at
 * the function `entry`, with argument `args`, using a stack whose top is
 * given by `stack_top`.
 *
 * @param entry     Function to execute when the context starts.
 * @param args      Argument passed to the entry function.
 * @param stack_top Pointer to the top of the allocated stack.
 * @return A new context_t representing the initialized context.
 */
context_t context_create(void (*entry)(void *), void *args, void *stack_top);

/**
 * @brief Switch from one context to another.
 *
 * High‑level wrapper around the assembly context switch routine.
 *
 * @param old_ctx Pointer to store the current context.
 * @param new_ctx Context to switch into.
 */
void context_switch(context_t *old_ctx, context_t new_ctx);

#endif /* CONTEXT_H */
