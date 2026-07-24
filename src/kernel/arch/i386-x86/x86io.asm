
; io.asm
; Low-level x86 I/O routines
; NASM

BITS 32

SECTION .text

global in8
global in16
global in32

global out8
global out16
global out32

global mmin8
global mmin16
global mmin32

global mmout8
global mmout16
global mmout32

global enableinterrupts
global disableinterrupts
global halt
global iowait

; u8 in8(u16 port)
in8:
    mov dx, [esp+4]
    xor eax, eax
    in al, dx
    ret

; u16 in16(u16 port)
in16:
    mov dx, [esp+4]
    xor eax, eax
    in ax, dx
    ret

; u32 in32(u16 port)
in32:
    mov dx, [esp+4]
    in eax, dx
    ret

; void out8(u16 port, u8 value)
out8:
    mov dx, [esp+4]
    mov al, [esp+8]
    out dx, al
    ret

; void out16(u16 port, u16 value)
out16:
    mov dx, [esp+4]
    mov ax, [esp+8]
    out dx, ax
    ret

; void out32(u16 port, u32 value)
out32:
    mov dx, [esp+4]
    mov eax, [esp+8]
    out dx, eax
    ret

; u8 mmin8(void *ptr)
mmin8:
    mov eax, [esp+4]
    movzx eax, byte [eax]
    ret

; u16 mmin16(void *ptr)
mmin16:
    mov eax, [esp+4]
    movzx eax, word [eax]
    ret

; u32 mmin32(void *ptr)
mmin32:
    mov eax, [esp+4]
    mov eax, [eax]
    ret

; void mmout8(void *ptr, u8 value)
mmout8:
    mov eax, [esp+4]
    mov dl, [esp+8]
    mov [eax], dl
    ret

; void mmout16(void *ptr, u16 value)
mmout16:
    mov eax, [esp+4]
    mov dx, [esp+8]
    mov [eax], dx
    ret

; void mmout32(void *ptr, u32 value)
mmout32:
    mov eax, [esp+4]
    mov edx, [esp+8]
    mov [eax], edx
    ret

; Enable interrupts
enableinterrupts:
    sti
    ret

; Disable interrupts
disableinterrupts:
    cli
    ret

; Halt CPU
halt:
    hlt
    ret

; I/O wait
iowait:
    mov dx, 0x80
    xor al, al
    out dx, al
    ret