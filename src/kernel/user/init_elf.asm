section .rodata

global user_init_elf_image
global user_init_elf_image_size

%ifndef USER_INIT_ELF
%define USER_INIT_ELF "build/init.elf"
%endif

user_init_elf_image:
    incbin USER_INIT_ELF
align 4
user_init_elf_image_size:
    dd ($ - user_init_elf_image)
