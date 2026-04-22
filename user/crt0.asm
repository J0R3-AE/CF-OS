; user/crt0.asm
BITS 32

GLOBAL _start
EXTERN main
EXTERN exit

SECTION .text
_start:
    ; call int main(void)
    call main

    ; if main returns, exit(ret)
    push eax
    call exit

.hang:
    hlt
    jmp .hang