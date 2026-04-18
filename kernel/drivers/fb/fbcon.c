

#include "libk/types.h"
#include "libk/string.h"

#include "drivers/fbcon.h"

#include "gui/font.h" // unsigned char font8x16[][16];

static u8 *fb;
static u32 fb_width;
static u32 fb_height;
static u32 fb_pitch;
static u32 fb_bpp;

#define FONT_W 8
#define FONT_H 16

static inline u32 fb_cols(void) { return fb_width / FONT_W; }
static inline u32 fb_rows(void) { return fb_height / FONT_H; }

size_t fbcon_cols(void)
{
    return fb_width / FONT_W;
}

size_t fbcon_rows(void)
{
    return fb_height / FONT_H;
}

/* cursor in character cells */
static u32 cursor_x = 0;
static u32 cursor_y = 0;

static void fb_putpixel(u32 x, u32 y, u32 color)
{
    if (x >= fb_width || y >= fb_height)
        return;

    u8 *p = fb + y * fb_pitch + x * (fb_bpp / 8);

    if (fb_bpp == 32)
    {
        *(u32 *)p = color;
    }
    else if (fb_bpp == 24)
    {
        p[0] = color & 0xFF;
        p[1] = (color >> 8) & 0xFF;
        p[2] = (color >> 16) & 0xFF;
    }
    else if (fb_bpp == 16)
    {
        u16 r = (color >> 19) & 0x1F;
        u16 g = (color >> 10) & 0x3F;
        u16 b = (color >> 3) & 0x1F;
        *(u16 *)p = (r << 11) | (g << 5) | b;
    }
}

static void fb_draw_char_px(u32 px, u32 py, char c, u32 fg, u32 bg)
{
    const u8 *glyph = font8x16[(u8)c];

    for (u32 row = 0; row < FONT_H; row++)
    {
        u8 bits = glyph[row];
        for (u32 col = 0; col < FONT_W; col++)
        {
            u32 color = (bits & (0x80 >> col)) ? fg : bg;
            fb_putpixel(px + col, py + row, color);
        }
    }
}

static void fb_scroll_internal(void)
{
    u32 row_bytes = fb_pitch * FONT_H;
    u32 screen_bytes = fb_pitch * fb_height;

    memmove(fb, fb + row_bytes, screen_bytes - row_bytes);
    memset(fb + screen_bytes - row_bytes, 0, row_bytes);

    if (cursor_y > 0)
        cursor_y--;
}

void fbcon_init(u32 virt_addr, u32 width, u32 height, u32 pitch, u32 bpp)
{
    fb        = (u8 *)virt_addr;
    fb_width  = width;
    fb_height = height;
    fb_pitch  = pitch;
    fb_bpp    = bpp;

    cursor_x = cursor_y = 0;
    fbcon_clear();
}

void fbcon_clear(void)
{
    memset(fb, 0, fb_pitch * fb_height);
    cursor_x = cursor_y = 0;
}

void fbcon_putc(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c == '\r')
    {
        cursor_x = 0;
    }
    else if (c == '\b')
    {
        if (cursor_x > 0)
            cursor_x--;
        fb_draw_char_px(cursor_x * FONT_W, cursor_y * FONT_H, ' ', 0xFFFFFF, 0x000000);
    }
    else
    {
        fb_draw_char_px(cursor_x * FONT_W, cursor_y * FONT_H, c, 0xFFFFFF, 0x000000);
        cursor_x++;
    }

    if (cursor_x >= fb_cols())
    {
        cursor_x = 0;
        cursor_y++;
    }
    if (cursor_y >= fb_rows())
    {
        fb_scroll_internal();
    }
}

void fbcon_write(const char *s, size_t len)
{
    for (size_t i = 0; i < len; i++)
        fbcon_putc(s[i]);
}

void fbcon_puts(const char *s)
{
    if (!s)
        return;
    while (*s)
        fbcon_putc(*s++);
}

/* TTY-style helpers */

void fbcon_clearline(size_t row)
{
    if (row >= fb_rows())
        return;

    u32 y = row * FONT_H;
    for (u32 r = 0; r < FONT_H; r++)
    {
        u8 *line = fb + (y + r) * fb_pitch;
        memset(line, 0, fb_pitch);
    }
}

void fbcon_putcat(size_t row, size_t col, char ch, u32 fg, u32 bg)
{
    if (row >= fb_rows() || col >= fb_cols())
        return;

    u32 px = col * FONT_W;
    u32 py = row * FONT_H;
    fb_draw_char_px(px, py, ch, fg, bg);
}

void fbcon_scroll(void)
{
    fb_scroll_internal();
}
