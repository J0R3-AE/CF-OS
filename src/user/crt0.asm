; crt0.asm (i386, ELF)
; Entry point for user programs

global _start
extern main

section .text
_start:
    ; The kernel trampoline calls the ELF entry as _start(argc, argv).
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    push edx
    push eax
    call main
    add esp, 8

    ; return value from main -> exit(status)
    mov ebx, eax      ; status
    mov eax, 1        ; SYS_exit 
    int 0x80
