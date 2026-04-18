#pragma once

#include "libk/types.h"

#define HEAP_START  0x00800000   // 8MB
#define HEAP_MAX    0x02000000   // 32MB

#define TOTAL_RAM        (128 * 1024 * 1024)   // QEMU default
#define KERNEL_END       0x00200000            // ~2MB
#define KERNEL_STACK_TOP 0xC03FF000

#define STACK_SIZE 4096


