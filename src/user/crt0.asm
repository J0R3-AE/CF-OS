; crt0.asm (i386, ELF)
; Entry point for user programs

global _start
extern main

section .text
_start:
    ; call main()
    call main

    ; return value from main -> exit(status)
    mov ebx, eax      ; status
    mov eax, 1        ; SYS_exit 
    int 0x80
