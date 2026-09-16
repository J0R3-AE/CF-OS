#include "kernel/drivers/wm.h"
#include "kernel/drivers/framebuffer.h"
#include "libc/mem.h"
#include "libc/types.h"
#include "libc/printf.h"

#include "kernel/mm/heap.h"
#include <stddef.h>

static void *saved_fb = NULL;
static usize saved_size = 0;
static int wm_visible = 0;

void wm_init(void)
{
    saved_fb = NULL;
    saved_size = 0;
    wm_visible = 0;
}

static void draw_rect(int x, int y, int w, int h, uint32_t color)
{
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            framebuffer_putpixel(xx, yy, color);
}

void wm_toggle(void)
{
    u32 w = framebuffer_width();
    u32 h = framebuffer_height();
    u32 pitch = framebuffer_pitch();

    if (!wm_visible)
    {
        /* capture framebuffer */
        if (saved_fb)
            return;

        saved_size = (usize)pitch * (usize)h;
        saved_fb = malloc(saved_size);
        if (!saved_fb)
            return;

        memcpy(saved_fb, (void *)framebuffer_address(), saved_size);

        /* simple overlay: semi-transparent dark background */
        for (u32 yy = 0; yy < h; ++yy)
            for (u32 xx = 0; xx < w; ++xx)
            {
                uint32_t src = ((uint32_t *)framebuffer_address())[yy * (pitch/4) + xx];
                /* blend with 0x000000 at 60% */
                uint32_t out = ((src & 0xFF) * 40 / 100) | (((src >> 8) & 0xFF) * 40 / 100 << 8) | (((src >> 16) & 0xFF) * 40 / 100 << 16);
                framebuffer_putpixel(xx, yy, out);
            }

        /* draw a debug window */
        int ww = w * 6 / 10;
        int hh = h * 6 / 10;
        int x = (w - ww) / 2;
        int y = (h - hh) / 2;

        /* window background */
        draw_rect(x, y, ww, hh, 0x202020);
        /* title bar */
        draw_rect(x, y, ww, 20, 0x404040);

        /* avoid printing to serial/TTY to keep overlay decoupled */

        wm_visible = 1;
    }
    else
    {
        /* restore framebuffer */
        if (saved_fb)
        {
            memcpy((void *)framebuffer_address(), saved_fb, saved_size);
            free(saved_fb);
            saved_fb = NULL;
            saved_size = 0;
        }

        wm_visible = 0;
    }
}

int wm_is_visible(void)
{
    return wm_visible;
}
