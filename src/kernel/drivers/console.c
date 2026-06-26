#include "drivers/console.h"
#include "drivers/framebuffer.h"
#include "gui/font.h"
#include "libk/mem.h"
#include <stdint.h>

#define FW 8
#define FH 16

static uint32_t fg = 0xFFFFFF;
static uint32_t bg = 0x000000;

static size_t cur_x = 0;
static size_t cur_y = 0;

/* size */
size_t console_rows(void) { return framebuffer_height() / FH; }
size_t console_cols(void) { return framebuffer_width() / FW; }

/* pixel */
static inline void putpixel(int x, int y, uint32_t c)
{
    framebuffer_putpixel(x, y, c);
}

/* draw char */
static void draw_char(int px, int py, char c, uint32_t fg, uint32_t bg)
{
    const uint8_t *g = font8x16[(uint8_t)c];

    for (int y = 0; y < FH; y++)
    {
        for (int x = 0; x < FW; x++)
        {
            uint32_t col = (g[y] & (0x80 >> x)) ? fg : bg;
            putpixel(px + x, py + y, col);
        }
    }
}

/* direct draw */
void console_putc_at(size_t row, size_t col, char c,
                     uint32_t fgc, uint32_t bgc)
{
    draw_char(col * FW, row * FH, c, fgc, bgc);
}

/* clear */
void console_clear(void)
{
    framebuffer_fill(bg);
    cur_x = 0;
    cur_y = 0;
}

/* scroll */
void console_scroll(void)
{
    uint32_t row_bytes = framebuffer_pitch() * FH;
    uint32_t size = framebuffer_pitch() * framebuffer_height();

    memmove((void *)framebuffer_address(), (void *)(framebuffer_address() + row_bytes), size - row_bytes);
    memset((void *)(framebuffer_address() + size - row_bytes), 0, row_bytes);

}

/* init */
void console_init(uint32_t addr, uint32_t w, uint32_t h,
                  uint32_t pitch, uint32_t bpp)
{
    framebuffer_init(addr, w, h, pitch, bpp);
    console_clear();
}

/* put char */
void console_putc(char c)
{
    if (c == '\n')
    {
        cur_x = 0;
        cur_y++;
        goto check_scroll;
    }

    if (c == '\r')
    {
        cur_x = 0;
        return;
    }

    if ((unsigned char)c < 32)
        return;

    console_putc_at(cur_y, cur_x, c, fg, bg);

    cur_x++;

    if (cur_x >= console_cols())
    {
        cur_x = 0;
        cur_y++;
    }

check_scroll:
    if (cur_y >= console_rows())
    {
        console_scroll();
        cur_y = console_rows() - 1;
    }
}

/* string */
void console_puts(const char *s)
{
    if (!s) return;
    while (*s) console_putc(*s++);
}