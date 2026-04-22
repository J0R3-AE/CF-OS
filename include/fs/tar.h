#pragma once

#include "libk/types.h"
#include "fs/vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tar_header
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
} __attribute__((packed));

int tar_extract(struct vnode *root, void *start, u32 size);

#ifdef __cplusplus
}
#endif