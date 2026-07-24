#include "kernel/drivers/framebuffer.h"

#include "libc/types.h"
#include <stddef.h>

static u8 *fb;
static u32 fb_width;
static u32 fb_height;
static u32 fb_pitch;
static u32 fb_bpp;

/* getters */
u32 framebuffer_width(void)  { return fb_width; }
u32 framebuffer_height(void) { return fb_height; }
u32 framebuffer_pitch(void)  { return fb_pitch; }
u32 framebuffer_bpp(void)    { return fb_bpp; }

uintptr_t framebuffer_address(void) { return (uintptr_t)fb; }

/* pixel writers */
static inline void put32(u8 *p, u32 c) { *(u32*)p = c; }

static inline void put24(u8 *p, u32 c)
{
    p[0] = c & 0xFF;
    p[1] = (c >> 8) & 0xFF;
    p[2] = (c >> 16) & 0xFF;
}

static inline void put16(u8 *p, u32 c)
{
    u16 r = (c >> 19) & 0x1F;
    u16 g = (c >> 10) & 0x3F;
    u16 b = (c >> 3) & 0x1F;
    *(u16*)p = (r << 11) | (g << 5) | b;
}

void framebuffer_init(u32 addr, u32 w, u32 h,
                      u32 pitch, u32 bpp)
{
    fb = (u8*)addr;
    fb_width = w;
    fb_height = h;
    fb_pitch = pitch;
    fb_bpp = bpp;

    framebuffer_fill(0x000000);
}

/* pixel */
void framebuffer_putpixel(int x, int y, u32 color)
{
    if (!fb) return;
    if ((u32)x >= fb_width || (u32)y >= fb_height) return;

    u8 *p = fb + y * fb_pitch + x * (fb_bpp / 8);

    switch (fb_bpp)
    {
        case 32: put32(p, color); break;
        case 24: put24(p, color); break;
        case 16: put16(p, color); break;
    }
}

/* clear */
void framebuffer_fill(u32 color)
{
    if (!fb) return;

    if (fb_bpp == 32)
    {
        u32 *p = (u32*)fb;
        u32 count = fb_width * fb_height;

        for (u32 i = 0; i < count; i++)
            p[i] = color;
    }
    else
    {
        for (u32 y = 0; y < fb_height; y++)
            for (u32 x = 0; x < fb_width; x++)
                framebuffer_putpixel(x, y, color);
    }
}