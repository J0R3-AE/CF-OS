/** @brief peen */

#include "kernel/arch/context.h"
#include "kernel/log.h"

extern void kthread_entry(void);

context_t context_create(void (*entry)(void*), void* args, void* stack_top)
{
    (void)entry;
    (void)args;

    u32* stk = (u32*)stack_top;
    u32 *frame_slot;

    /*
     * The initial context reaches kthread_entry through the same leave/ret
     * sequence used when a suspended context_switch resumes.
     *
     * popal
     * popfl
     * leave
     * ret
     */

    // leave's frame: saved EBP followed by the return target.
    *(--stk) = (u32)kthread_entry;
    *(--stk) = 0;
    frame_slot = stk;

    // EFLAGS for popfl
    *(--stk) = 0x202;

    // pushal frame in reverse order for popal
    *(--stk) = 0; // EAX
    *(--stk) = 0; // ECX
    *(--stk) = 0; // EDX
    *(--stk) = 0; // EBX
    *(--stk) = 0; // ESP dummy
    *(--stk) = (u32)frame_slot; // EBP used by leave
    *(--stk) = 0; // ESI
    *(--stk) = 0; // EDI

    return stk;
}
void context_switch(context_t* old_ctx, context_t new_ctx) {
    asm volatile(
        "pushfl\n\t"          // save EFLAGS
        "pushal\n\t"          // save GPRs

        "test %0, %0\n\t"
        "jz 1f\n\t"
        "movl %%esp, (%0)\n\t" // *old_ctx = esp
        "1:\n\t"

        "movl %1, %%esp\n\t"   // esp = new_ctx

        "popal\n\t"            // restore GPRs
        "popfl\n\t"            // restore EFLAGS
        "leave\n\t"            // restore caller's frame pointer
        "ret\n\t"              // jump to entry
        :
        : "r"(old_ctx), "r"(new_ctx)
        : "memory"
    );
}