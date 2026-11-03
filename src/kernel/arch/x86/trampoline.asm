BITS 32

section .text

global kthread_entry

extern sched_current
extern thread_exit

kthread_entry:

    ; Get current thread from the new scheduler.
    call sched_current

    ; eax = thread_t*
    test eax, eax
    jz .dead

    ; entry
    mov edx, [eax + 36]

    ; arg
    mov ecx, [eax + 40]

    ; Call entry(arg)
    push ecx
    call edx
    add esp, 4

    ; If the entry function returns, terminate the thread.
    call thread_exit

.dead:
    cli

.hang:
    hlt
    jmp .hang