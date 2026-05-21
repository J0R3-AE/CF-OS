global kthread_entry

extern g_current
extern thread_exit

section .text

kthread_entry:
    ; Get the current thread pointer from the scheduler state.
    mov eax, [g_current]

    ; Thread struct layout on 32-bit MiniOS:
    ;   Link run_link      (8 bytes)
    ;   context_t ctx      (4 bytes)
    ;   u8 *stack          (4 bytes)
    ;   size_t stack_size  (4 bytes)
    ;   thread_state_t state (4 bytes)
    ;   u64 wakeup_tick    (8 bytes)
    ;   Thread *next_sleep (4 bytes)
    ;   void (*entry)(void *) (4 bytes)
    ;   void *arg          (4 bytes)
    mov ebx, [eax + 36]    ; entry function pointer
    mov ecx, [eax + 40]    ; argument

    push ecx
    call ebx

    ; If thread returns, exit cleanly back to the scheduler.
    call thread_exit

.hang:
    cli
    hlt
    jmp .hang