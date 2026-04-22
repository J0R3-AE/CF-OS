#pragma once
#include "libk/types.h"
#include "fs/blockdev.h"

struct ramdisk_device {
    void *image;
    u32 size;
};

struct blockdev *ramdisk_init(void *data, u32 size, const char *name);