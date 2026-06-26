section .rodata

global user_init_elf_image
global user_init_elf_image_size
user_init_elf_image:
    incbin "build/init.elf"
align 4
user_init_elf_image_size:
    dd ($ - user_init_elf_image)
