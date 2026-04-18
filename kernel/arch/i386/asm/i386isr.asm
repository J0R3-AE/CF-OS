; src/kernel/arch/i386/i386isr.asm
; NASM ISR / IRQ stubs - expose both isrN and isr_N names
[BITS 32]
SECTION .text

EXTERN isr_handler
EXTERN irq_handler

; Macro: ISR without error code
; Creates labels: isrN and isr_N (both point to same code)
%macro ISR_NOERRCODE 1
    GLOBAL isr%1
    GLOBAL isr_%1
isr%1:
isr_%1:
    cli
    push dword 0         ; dummy error code pushed so handler has uniform stack layout
    push dword %1        ; push interrupt number
    jmp isr_common_stub
%endmacro

; Macro: ISR with error code
; CPU already pushes an error code for these exceptions — we only push the int number
%macro ISR_ERRCODE 1
    GLOBAL isr%1
    GLOBAL isr_%1
isr%1:
isr_%1:
    cli
    push dword %1        ; push interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; Macro: IRQ stub
%macro IRQ 2
    GLOBAL irq%1
    GLOBAL irq_%1
irq%1:
irq_%1:
    cli
    push dword 0         ; dummy error code
    push dword %2        ; push mapped interrupt number (IDT number)
    jmp irq_common_stub
%endmacro

; ---------------------------
; CPU Exception ISRs 0..31
; ---------------------------
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31
ISR_NOERRCODE 128


; ---------------------------
; Hardware IRQs 0..15 -> IDT numbers 32..47
; ---------------------------
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; ---------------------------
; Common ISR stub
; ---------------------------
isr_common_stub:
    pusha               ; push eax..edi
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10        ; kernel data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
push eax
call isr_handler
add esp, 4

    ; restore segments and registers (reverse order)
    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 8          ; pop error code and int number
    sti
    iretd

; ---------------------------
; Common IRQ stub
; ---------------------------
irq_common_stub:
    pusha
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
push eax
call irq_handler
add esp, 4

    pop gs
    pop fs
    pop es
    pop ds
    popa

    add esp, 8
    sti
    iretd