[BITS 32]

SECTION .multiboot
ALIGN 4

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x00000007        ; mem info + boot device + video
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

; Optional address fields (we'll leave them 0 for now)
    ; framebuffer tag
    dw 5                        ; tag type: framebuffer
    dw 0                        ; flags
    dd 20                       ; size
    dd 1024                     ; width
    dd 768                      ; height
    dd 32                       ; bpp

    ; end tag
    dw 0
    dw 0
    dd 8