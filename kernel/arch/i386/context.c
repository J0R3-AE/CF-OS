/** @brief peen */

#include "arch/context.h"
#include "libk/log.h"

context_t context_create(void (*entry)(void*), void* args, void* stack_top) {
    u32* stk = (u32*)stack_top;

    // Push return address for 'ret' in context_switch
    *(--stk) = (u32)entry;      // return address (EIP after ret)

    // Push EFLAGS (will be restored by popfl)
    *(--stk) = 0x202;           // EFLAGS (IF=1)

    // Push general-purpose registers for popal
    *(--stk) = (u32)args;       // EAX (argument)
    *(--stk) = 0;               // ECX
    *(--stk) = 0;               // EDX
    *(--stk) = 0;               // EBX
    *(--stk) = 0;               // ESP (ignored by popal)
    *(--stk) = 0;               // EBP
    *(--stk) = 0;               // ESI
    *(--stk) = 0;               // EDI

    klog_log("Context created for entry %p with args %p, stack top %p", (void*)entry, args, (void*)stack_top);
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
        "ret\n\t"              // jump to entry
        :
        : "r"(old_ctx), "r"(new_ctx)
        : "memory"
    );
    klog_log("Context switched to new context at %p", (void*)new_ctx);
}