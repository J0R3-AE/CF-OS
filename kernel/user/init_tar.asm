section .rodata

global user_init_tar_image
global user_init_tar_image_size
user_init_tar_image:
    incbin "user/build/init.tar"
align 4
user_init_tar_image_size:
    dd ($ - user_init_tar_image)
