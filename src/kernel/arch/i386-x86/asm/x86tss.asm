[BITS 32]
global tss_flush

tss_flush:
    mov ax, [esp + 4]   ; selector (16-bit)
    ltr ax
    ret
