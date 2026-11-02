#pragma once
#include "libc/types.h"

bool framebuffer_init(uptr addr, u32 w, u32 h, u32 pitch, u32 bpp);
bool framebuffer_is_initialized(void);

void framebuffer_putpixel(int x, int y, u32 color);
void framebuffer_fill(u32 color);
void framebuffer_scroll(u32 pixel_rows, u32 background);

u32 framebuffer_width(void);
u32 framebuffer_height(void);
u32 framebuffer_pitch(void);
u32 framebuffer_bpp(void);
uptr framebuffer_address(void);
