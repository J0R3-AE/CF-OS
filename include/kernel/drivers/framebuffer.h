#pragma once
#include "libc/types.h"
#include <stdint.h>
#include <stddef.h>

void framebuffer_init(u32 addr, u32 w, u32 h,
                      u32 pitch, u32 bpp);

void framebuffer_putpixel(int x, int y, u32 color);
void framebuffer_fill(u32 color);

u32 framebuffer_width(void);
u32 framebuffer_height(void);
u32 framebuffer_pitch(void);
u32 framebuffer_bpp(void);
uintptr_t framebuffer_address(void);