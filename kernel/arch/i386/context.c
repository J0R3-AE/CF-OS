/** @brief peen */

#include "arch/context.h"
#include "libk/log.h"

extern void kthread_entry(void);

context_t context_create(void (*entry)(void*), void* args, void* stack_top) {
    (void)entry;
    (void)args;

    u32* stk = (u32*)stack_top;

    // Push the return address for context_switch. The trampoline will
    // obtain the current thread from g_current and invoke its entry.
    *(--stk) = (u32)kthread_entry;

    // Push EFLAGS (will be restored by popfl)
    *(--stk) = 0x202;           // EFLAGS (IF=1)

    // Push a fake pushal frame. The context_switch implementation does
    // popal/popfl/ret, so the stack must look like a saved pushal frame.
    *(--stk) = 0;               // EAX
    *(--stk) = 0;               // ECX
    *(--stk) = 0;               // EDX
    *(--stk) = 0;               // EBX
    u32 *ebx_slot = stk;
    *(--stk) = (u32)ebx_slot;   // ESP placeholder points to saved EBX
    *(--stk) = 0;               // EBP
    *(--stk) = 0;               // ESI
    *(--stk) = 0;               // EDI

    return stk;
}

void context_switch(context_t* old_ctx, context_t new_ctx) {
    KLOG_INFO("context_switch: old_ctx=%p new_ctx=%p", old_ctx, new_ctx);
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
        "ret\n\t"              // jump to entry
        :
        : "r"(old_ctx), "r"(new_ctx)
        : "memory"
    );
}