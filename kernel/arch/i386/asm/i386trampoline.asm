global kthread_entry

extern g_current
extern thread_exit

section .text

kthread_entry:
    ; get current thread
    mov eax, [g_current]

    ; load entry + arg from thread
    mov ebx, [eax + 0]     ; entry function pointer
    mov ecx, [eax + 4]     ; argument

    push ecx
    call ebx

    ; if thread returns → exit cleanly
    call thread_exit

.hang:
    cli
    hlt
    jmp .hang