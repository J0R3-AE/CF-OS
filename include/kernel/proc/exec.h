#pragma once

#include "libc/types.h"
struct vnode;

int proc_exec_vnode(struct vnode *vn);

int exec_elf_vnode(struct vnode *vn);

int exec_elf_image(
    const void *image,
    u32 size);