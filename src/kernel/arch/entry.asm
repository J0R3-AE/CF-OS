; src/kernel/entry.asm
[BITS 32]
GLOBAL _start
extern kmain

SECTION .text
_start:
    cli                     ; disable interrupts while we set up

    ; Set up a simple stack (some space below 1MB)
    mov esp, 0x9FB00
    mov ebp, esp

    push ebx            ; multiboot_info*
    push eax            ; magic (optional)

    ; Call C kernel entry
    call kmain

.hang:
    hlt
    jmp .hang
