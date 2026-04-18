#pragma once
#include "libk/types.h"

/**
 * @brief Saved CPU context pointer (old stack frame).
 *
 * This is what trampoline_start passes to trampoline_end.
 */
typedef u32 *trampoline_ctx_t;

/**
 * @brief Save current CPU state, switch to a new stack, and call target().
 *
 * This:
 *  - saves all GPRs + EFLAGS on the old stack
 *  - switches to new_stack
 *  - passes a pointer to the saved state to trampoline_end()
 *  - calls target()
 *
 * When target() eventually wants to resume the old context, it should
 * call trampoline_end(ctx).
 *
 * @param target    Function to run on the new stack.
 * @param new_stack New stack pointer (top of stack).
 */
void trampoline_start(void (*target)(void), void *new_stack);

/**
 * @brief Restore a previously saved CPU state and return to it.
 *
 * This:
 *  - switches back to the old stack
 *  - restores EFLAGS + all GPRs
 *  - returns to the original caller of trampoline_start()
 *
 * @param ctx Pointer previously provided by trampoline_start().
 */
void trampoline_end(trampoline_ctx_t ctx) __attribute__((noreturn));

/**
 * @brief Non‑returning jump: set ESP and jump to entry.
 *
 * Pure control transfer: no state save/restore, just:
 *  - optional CLI
 *  - ESP = new_stack
 *  - JMP entry
 *
 * @param entry      Target address to jump to.
 * @param new_stack  New stack pointer.
 */
void i386trampoline_jump(u32 entry, u32 new_stack) __attribute__((noreturn));
