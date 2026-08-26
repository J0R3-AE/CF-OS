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

    ; New thread_t layout:
    ;
    ; u32 tid                    0
    ; thread_state_t state       4
    ; context_t context          8
    ; void *stack                12
    ; usize stack_size           16
    ; u64 wakeup_tick            20
    ; struct process *process    28
    ; void (*entry)(void *)      32
    ; void *arg                  36
    ; bool queued                40
    ; thread_t *next             44
    ;
    ; entry
    mov edx, [eax + 32]

    ; arg
    mov ecx, [eax + 36]

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