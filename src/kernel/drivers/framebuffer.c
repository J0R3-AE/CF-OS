#include "kernel/drivers/framebuffer.h"

static volatile u8 *fb;
static u32 fb_width;
static u32 fb_height;
static u32 fb_pitch;
static u32 fb_bpp;

u32 framebuffer_width(void) { return fb_width; }
u32 framebuffer_height(void) { return fb_height; }
u32 framebuffer_pitch(void) { return fb_pitch; }
u32 framebuffer_bpp(void) { return fb_bpp; }
uptr framebuffer_address(void) { return (uptr)fb; }
bool framebuffer_is_initialized(void) { return fb != NULL; }

bool framebuffer_init(uptr address, u32 width, u32 height, u32 pitch, u32 bpp)
{
    u32 bytes_per_pixel;
    u64 required;

    if (!address || !width || !height ||
        (bpp != 16 && bpp != 24 && bpp != 32))
        return false;

    bytes_per_pixel = bpp / 8;
    if ((u64)width * bytes_per_pixel > pitch)
        return false;

    required = (u64)pitch * height;
    if (!required || required > 0xFFFFFFFFULL ||
        (u64)address + required > 0x100000000ULL)
        return false;

    fb = (volatile u8 *)address;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;
    fb_bpp = bpp;
    framebuffer_fill(0);
    return true;
}

void framebuffer_putpixel(int x, int y, u32 color)
{
    volatile u8 *pixel;
    u8 red, green, blue;

    if (!fb || x < 0 || y < 0 ||
        (u32)x >= fb_width || (u32)y >= fb_height)
        return;

    pixel = fb + (u32)y * fb_pitch + (u32)x * (fb_bpp / 8);
    red = (u8)(color >> 16);
    green = (u8)(color >> 8);
    blue = (u8)color;

    if (fb_bpp == 16)
    {
        u16 value = (u16)(((u16)(red >> 3) << 11) |
                          ((u16)(green >> 2) << 5) |
                          (blue >> 3));
        pixel[0] = (u8)value;
        pixel[1] = (u8)(value >> 8);
    }
    else
    {
        pixel[0] = blue;
        pixel[1] = green;
        pixel[2] = red;
        if (fb_bpp == 32)
            pixel[3] = 0;
    }
}

void framebuffer_fill(u32 color)
{
    if (!fb)
        return;

    for (u32 y = 0; y < fb_height; y++)
        for (u32 x = 0; x < fb_width; x++)
            framebuffer_putpixel((int)x, (int)y, color);
}

void framebuffer_scroll(u32 pixel_rows, u32 background)
{
    u32 copy_bytes;
    u32 clear_start;

    if (!fb || !pixel_rows)
        return;

    if (pixel_rows >= fb_height)
    {
        framebuffer_fill(background);
        return;
    }

    copy_bytes = (fb_height - pixel_rows) * fb_pitch;
    clear_start = fb_height - pixel_rows;
    for (u32 i = 0; i < copy_bytes; i++)
        fb[i] = fb[i + pixel_rows * fb_pitch];

    for (u32 y = clear_start; y < fb_height; y++)
        for (u32 x = 0; x < fb_width; x++)
            framebuffer_putpixel((int)x, (int)y, background);
}
