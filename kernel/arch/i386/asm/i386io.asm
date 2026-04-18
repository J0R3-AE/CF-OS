; i386_io_32.asm
; NASM, 32-bit (protected mode), cdecl calling convention.
; - cdecl: args on stack. return value in EAX.

[BITS 32]

GLOBAL i386io_inb
GLOBAL i386io_outb
GLOBAL i386io_enableinterrupts
GLOBAL i386io_dissableinterrupts
GLOBAL i386io_disableinterrupts
GLOBAL i386io_panic
GLOBAL i386io_iowait

GLOBAL i386io_inw
GLOBAL i386io_outw
GLOBAL i386io_inl
GLOBAL i386io_outl
GLOBAL i386io_hlt

; uint8_t i386_inb(uint16_t port)
i386io_inb:
    push ebp
    mov ebp, esp
    mov edx, [ebp+8]       ; port (32-bit on stack) -> dx low 16 bits
    in al, dx
    movzx eax, al
    pop ebp
    ret

; void i386_outb(uint16_t port, uint8_t value)
i386io_outb:
    push ebp
    mov ebp, esp
    mov edx, [ebp+8]       ; port
    mov eax, [ebp+12]      ; value (dword on stack)
    out dx, al
    pop ebp
    ret

; uint16_t i386_inw(uint16_t port)
i386io_inw:
    push ebp
    mov ebp, esp
    mov edx, [ebp+8]
    in ax, dx
    movzx eax, ax
    pop ebp
    ret

; void i386_outw(uint16_t port, uint16_t value)
i386io_outw:
    push ebp
    mov ebp, esp
    mov edx, [ebp+8]
    mov eax, [ebp+12]
    out dx, ax
    pop ebp
    ret

; uint32_t i386_inl(uint16_t port)
i386io_inl:
    push ebp
    mov ebp, esp
    mov edx, [ebp+8]
    in eax, dx
    pop ebp
    ret

; void i386_outl(uint16_t port, uint32_t value)
i386io_outl:
    push ebp
    mov ebp, esp
    mov edx, [ebp+8]
    mov eax, [ebp+12]
    out dx, eax
    pop ebp
    ret

; void i386_enableinterrupts(void)
i386io_enableinterrupts:
    sti
    ret

; void i386_disableinterrupts(void)
i386io_disableinterrupts:
    cli
    ret

; void i386_hlt(void)
i386io_hlt:
    hlt
    ret

; void i386_iowait(void)
; simple I/O wait using outb to port 0x80 (traditional)
i386io_iowait:
    push eax
    mov al, 0
    out 0x80, al
    pop eax
    ret

; void i386_panic(const char *msg)
; Very small VGA text write (writes at 0xb8000) then halt forever.
; Note: this assumes identity-mapped VGA memory (typical in early boot / protected mode).
i386io_panic:
    push ebp
    mov ebp, esp
    push esi
    push edi

    mov esi, [ebp+8]        ; esi = pointer to message (C string)
    mov edi, 0xB8000        ; video memory address
    mov ecx, 0              ; column counter

.panic_loop:
    mov al, [esi]
    cmp al, 0
    je .panic_done
    ; write character with attribute 0x0F (white on black)
    mov ah, 0x0F
    mov [edi], ax
    add edi, 2
    inc esi
    jmp .panic_loop

.panic_done:
    cli
.halt_loop:
    hlt
    jmp .halt_loop

    pop edi
    pop esi
    pop ebp
    ret
