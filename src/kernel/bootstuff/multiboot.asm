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
dd 0                  ; header_addr
dd 0                  ; load_addr
dd 0                  ; load_end_addr
dd 0                  ; bss_end_addr
dd 0                  ; entry_addr

; Video mode request
dd 0
dd 0
dd 0