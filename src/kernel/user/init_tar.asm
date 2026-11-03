section .rodata

global user_init_tar_image
global user_init_tar_image_size

%ifndef USER_INIT_TAR
%define USER_INIT_TAR "build/init.tar"
%endif

user_init_tar_image:
    incbin USER_INIT_TAR
align 4
user_init_tar_image_size:
    dd ($ - user_init_tar_image)
