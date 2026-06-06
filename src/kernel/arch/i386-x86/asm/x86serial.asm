; src/arch/i386/serial.asm
[BITS 32]
GLOBAL i386SERIAL_init
GLOBAL i386SERIAL_write
GLOBAL i386SERIAL_writestr

COM1_PORT equ 0x3F8

; Initialize COM1
i386SERIAL_init:
    mov dx, COM1_PORT + 1
    mov al, 0x00        ; disable interrupts
    out dx, al

    mov dx, COM1_PORT + 3
    mov al, 0x80        ; enable DLAB
    out dx, al

    mov dx, COM1_PORT
    mov al, 0x03        ; 38400 baud, low byte
    out dx, al
    mov dx, COM1_PORT + 1
    mov al, 0x00        ; high byte
    out dx, al

    mov dx, COM1_PORT + 3
    mov al, 0x03        ; 8 bits, no parity, 1 stop bit
    out dx, al

    mov dx, COM1_PORT + 2
    mov al, 0xC7        ; enable FIFO
    out dx, al

    ret

; void serial_write(char c)
i386SERIAL_write:
    pusha
.wait:
    mov dx, COM1_PORT + 5
    in al, dx
    test al, 0x20
    jz .wait
    mov dx, COM1_PORT
    mov al, [esp+36]
    out dx, al
    popa
    ret

; void serial_write_str(char* str)
i386SERIAL_writestr:
    pusha
    mov esi, [esp+36]
.next:
    lodsb
    test al, al
    jz .done
    push eax
    call i386SERIAL_write
    pop eax
    jmp .next
.done:
    popa
    ret
