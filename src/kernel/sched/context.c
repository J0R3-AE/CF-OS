#include "kernel/sched/context.h"
#include "kernel/sched/sched.h"

extern void kthread_entry(void);

int context_create(
    context_t *ctx,
    void (*entry)(void *),
    void *arg,
    void *stack_top)
{
    if (!ctx || !entry || !stack_top)
        return -1;

    /*
     * kthread_entry() obtains the current thread from
     * sched_current() and invokes thread->entry(thread->arg).
     *
     * Keep entry/arg here for the API, but the trampoline
     * owns the actual argument dispatch.
     */
    (void)entry;
    (void)arg;

    u32 *stk = (u32 *)stack_top;
    u32 *esp_slot;

    /*
     * Frame consumed by:
     *
     *     popad
     *     popfd
     *     pop ebp
     *     ret
     *
     * Layout from low to high:
     *
     *     EAX
     *     ECX
     *     EDX
     *     EBX
     *     ESP dummy
     *     EBP
     *     ESI
     *     EDI
     *     EFLAGS
     *     saved EBP
     *     return address
     */

    /* RET target */
    *(--stk) = (u32)kthread_entry;

    /* Saved EBP consumed by the compiler-generated function epilogue. */
    *(--stk) = 0;

    /* EFLAGS */
    *(--stk) = 0x202;

    /* POPAD frame */
    *(--stk) = 0; /* EDI */
    *(--stk) = 0; /* ESI */
    *(--stk) = 0; /* EBP */

    /*
     * ESP slot is ignored by POPAD.
     * Keep the old implementation's behavior.
     */
    esp_slot = stk;
    *(--stk) = (u32)esp_slot;

    *(--stk) = 0; /* EBX */
    *(--stk) = 0; /* EDX */
    *(--stk) = 0; /* ECX */
    *(--stk) = 0; /* EAX */

    ctx->esp = (u32)stk;

    return 0;
}

void context_switch(
    context_t *old_ctx,
    const context_t *new_ctx)
{
    __asm__ volatile(
        "pushfl\n\t"
        "pushal\n\t"

        "test %0, %0\n\t"
        "jz 1f\n\t"

        "movl %%esp, (%0)\n\t"

        "1:\n\t"

        "movl %1, %%esp\n\t"

        "popal\n\t"
        "popfl\n\t"
        :
        : "r"(old_ctx),
          "r"(new_ctx->esp)
        : "memory"
    );
}