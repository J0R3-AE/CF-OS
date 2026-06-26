#include "drivers/framebuffer.h"
#include <stdint.h>
#include <stddef.h>

static uint8_t *fb;
static uint32_t fb_width;
static uint32_t fb_height;
static uint32_t fb_pitch;
static uint32_t fb_bpp;

/* getters */
uint32_t framebuffer_width(void)  { return fb_width; }
uint32_t framebuffer_height(void) { return fb_height; }
uint32_t framebuffer_pitch(void)  { return fb_pitch; }
uint32_t framebuffer_bpp(void)    { return fb_bpp; }

uintptr_t framebuffer_address(void) { return (uintptr_t)fb; }

/* pixel writers */
static inline void put32(uint8_t *p, uint32_t c) { *(uint32_t*)p = c; }

static inline void put24(uint8_t *p, uint32_t c)
{
    p[0] = c & 0xFF;
    p[1] = (c >> 8) & 0xFF;
    p[2] = (c >> 16) & 0xFF;
}

static inline void put16(uint8_t *p, uint32_t c)
{
    uint16_t r = (c >> 19) & 0x1F;
    uint16_t g = (c >> 10) & 0x3F;
    uint16_t b = (c >> 3) & 0x1F;
    *(uint16_t*)p = (r << 11) | (g << 5) | b;
}

void framebuffer_init(uint32_t addr, uint32_t w, uint32_t h,
                      uint32_t pitch, uint32_t bpp)
{
    fb = (uint8_t*)addr;
    fb_width = w;
    fb_height = h;
    fb_pitch = pitch;
    fb_bpp = bpp;

    framebuffer_fill(0x000000);
}

/* pixel */
void framebuffer_putpixel(int x, int y, uint32_t color)
{
    if (!fb) return;
    if ((uint32_t)x >= fb_width || (uint32_t)y >= fb_height) return;

    uint8_t *p = fb + y * fb_pitch + x * (fb_bpp / 8);

    switch (fb_bpp)
    {
        case 32: put32(p, color); break;
        case 24: put24(p, color); break;
        case 16: put16(p, color); break;
    }
}

/* clear */
void framebuffer_fill(uint32_t color)
{
    if (!fb) return;

    if (fb_bpp == 32)
    {
        uint32_t *p = (uint32_t*)fb;
        uint32_t count = fb_width * fb_height;

        for (uint32_t i = 0; i < count; i++)
            p[i] = color;
    }
    else
    {
        for (uint32_t y = 0; y < fb_height; y++)
            for (uint32_t x = 0; x < fb_width; x++)
                framebuffer_putpixel(x, y, color);
    }
}