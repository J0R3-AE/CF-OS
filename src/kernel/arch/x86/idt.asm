[BITS 32]

SECTION .text
GLOBAL i386IDT_flush

i386IDT_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret