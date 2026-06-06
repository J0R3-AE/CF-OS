/*
 * Text-mode TTY with optional framebuffer backend.
 * VGA path writes to 0xB8000, fb path delegates all drawing to fbcon.
 */

#include "drivers/tty.h"
#include "drivers/fbcon.h"
#include "arch/io.h"
#include <stddef.h>
#include <stdint.h>

/* globals */
volatile uint16_t *TTY_base = (volatile uint16_t *)VGA_MEMORY;
size_t TTY_row = 0;
size_t TTY_col = 0;
size_t TTY_ROWS = 25;
size_t TTY_COLS = 80;
uint8_t TTY_fg = TTY_COLOR_LIGHT_GREY;
uint8_t TTY_bg = TTY_COLOR_BLACK;

static int TTY_use_fb = 0;

void TTY_set_fb_backend(int enable)
{
    TTY_use_fb = enable ? 1 : 0;
}

/* VGA entry helper */
static inline uint16_t make_entry(char ch, uint8_t fg, uint8_t bg)
{
    uint16_t attr = (uint16_t)(((bg & 0x0F) << 4) | (fg & 0x0F));
    return (uint16_t)((attr << 8) | (uint8_t)ch);
}

/* VGA hw cursor; no-op in fb mode */
static void TTY_update_hwcurs(void)
{
    if (TTY_use_fb)
        return;

    uint16_t pos = (uint16_t)(TTY_row * TTY_COLS + TTY_col);
    io_Write8(0x3D4, 0x0F);
    io_Write8(0x3D5, (uint8_t)(pos & 0xFF));
    io_Write8(0x3D4, 0x0E);
    io_Write8(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* VGA color index -> RGB for fbcon */
static uint32_t tty_color_to_rgb(uint8_t c)
{
    switch (c & 0x0F)
    {
    case 0:
        return 0x000000;
    case 1:
        return 0x0000AA;
    case 2:
        return 0x00AA00;
    case 3:
        return 0x00AAAA;
    case 4:
        return 0xAA0000;
    case 5:
        return 0xAA00AA;
    case 6:
        return 0xAA5500;
    case 7:
        return 0xAAAAAA;
    case 8:
        return 0x555555;
    case 9:
        return 0x5555FF;
    case 10:
        return 0x55FF55;
    case 11:
        return 0x55FFFF;
    case 12:
        return 0xFF5555;
    case 13:
        return 0xFF55FF;
    case 14:
        return 0xFFFF55;
    case 15:
        return 0xFFFFFF;
    default:
        return 0xFFFFFF;
    }
}

/* scroll one line up */
static void TTY_scroll_internal(void)
{
    if (TTY_use_fb)
    {
        fbcon_scroll();
        if (TTY_row > 0)
            TTY_row--;
        return;
    }

    volatile uint16_t *v = TTY_base;
    for (size_t r = 1; r < TTY_ROWS; ++r)
    {
        size_t src = r * TTY_COLS;
        size_t dst = (r - 1) * TTY_COLS;
        for (size_t c = 0; c < TTY_COLS; ++c)
            v[dst + c] = v[src + c];
    }
    uint16_t blank = make_entry(' ', TTY_fg, TTY_bg);
    size_t last = (TTY_ROWS - 1) * TTY_COLS;
    for (size_t c = 0; c < TTY_COLS; ++c)
        v[last + c] = blank;
}

/* PUBLIC API */

void TTY_init(void)
{
    if (TTY_use_fb)
    {
        TTY_ROWS = fbcon_rows();
        TTY_COLS = fbcon_cols();
    }
    else
    {
        TTY_ROWS = 25;
        TTY_COLS = 80;
        TTY_base = (volatile uint16_t *)VGA_MEMORY;
    }

    TTY_row = 0;
    TTY_col = 0;
    TTY_setcolor(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    TTY_clear();
    TTY_update_hwcurs();
}

void TTY_set_TTY_base(volatile uint16_t *addr)
{
    TTY_base = addr;
}

void TTY_clear(void)
{
    if (TTY_use_fb)
    {
        fbcon_clear();
        TTY_row = TTY_col = 0;
        return;
    }

    volatile uint16_t *v = TTY_base;
    uint16_t blank = make_entry(' ', TTY_fg, TTY_bg);
    for (size_t i = 0; i < TTY_ROWS * TTY_COLS; ++i)
        v[i] = blank;

    TTY_row = 0;
    TTY_col = 0;
    TTY_update_hwcurs();
}

void TTY_clearline(size_t row)
{
    if (row >= TTY_ROWS)
        return;

    if (TTY_use_fb)
    {
        fbcon_clearline(row);
        return;
    }

    volatile uint16_t *v = TTY_base;
    uint16_t blank = make_entry(' ', TTY_fg, TTY_bg);
    size_t base = row * TTY_COLS;
    for (size_t c = 0; c < TTY_COLS; ++c)
        v[base + c] = blank;
}

void TTY_setcolor(uint8_t fg, uint8_t bg)
{
    TTY_fg = fg & 0x0F;
    TTY_bg = bg & 0x0F;
}

void TTY_getcolor(uint8_t *fg, uint8_t *bg)
{
    if (fg)
        *fg = TTY_fg;
    if (bg)
        *bg = TTY_bg;
}

void TTY_resetcolor(void)
{
    TTY_fg = TTY_COLOR_WHITE;
    TTY_bg = TTY_COLOR_BLACK;
}

void TTY_getpos(size_t *row, size_t *col)
{
    if (row)
        *row = TTY_row;
    if (col)
        *col = TTY_col;
}

void TTY_getsize(size_t *rows, size_t *cols)
{
    if (rows)
        *rows = TTY_ROWS;
    if (cols)
        *cols = TTY_COLS;
}

size_t TTY_getrow(void) { return TTY_row; }
size_t TTY_getcol(void) { return TTY_col; }

void TTY_setrow(size_t row)
{
    if (row >= TTY_ROWS)
        row = TTY_ROWS - 1;
    TTY_row = row;
    TTY_update_hwcurs();
}

void TTY_setcol(size_t col)
{
    if (col >= TTY_COLS)
        col = TTY_COLS - 1;
    TTY_col = col;
    TTY_update_hwcurs();
}

void TTY_setpos(size_t row, size_t col)
{
    if (row >= TTY_ROWS)
        row = TTY_ROWS - 1;
    if (col >= TTY_COLS)
        col = TTY_COLS - 1;
    TTY_row = row;
    TTY_col = col;
    TTY_update_hwcurs();
}

/* low-level: write at position without moving software cursor */
void TTY_putcat(size_t row, size_t col, char ch, uint8_t fg, uint8_t bg)
{
    if (row >= TTY_ROWS || col >= TTY_COLS)
        return;

    if (TTY_use_fb)
    {
        uint32_t rfg = tty_color_to_rgb(fg);
        uint32_t rbg = tty_color_to_rgb(bg);
        fbcon_putcat(row, col, ch, rfg, rbg);
        return;
    }

    volatile uint16_t *v = TTY_base;
    v[row * TTY_COLS + col] = make_entry(ch, fg, bg);
}

/* write string at pos without moving software cursor */
void TTY_putsat(size_t row, size_t col, const char *s, uint8_t fg, uint8_t bg)
{
    if (!s)
        return;
    size_t r = row, c = col;
    for (const char *p = s; *p; ++p)
    {
        if (c >= TTY_COLS)
        {
            c = 0;
            ++r;
        }
        if (r >= TTY_ROWS)
            break;
        TTY_putcat(r, c, *p, fg, bg);
        ++c;
    }
}

/* internal char write at current cursor */
static void TTY_putch_internal(char ch)
{
    if (TTY_use_fb)
    {
        uint32_t fg = tty_color_to_rgb(TTY_fg);
        uint32_t bg = tty_color_to_rgb(TTY_bg);
        fbcon_putcat(TTY_row, TTY_col, ch, fg, bg);
        return;
    }

    volatile uint16_t *v = TTY_base;
    v[TTY_row * TTY_COLS + TTY_col] = make_entry(ch, TTY_fg, TTY_bg);
}

/* advance software cursor, scroll if needed */
static void TTY_advance(void)
{
    TTY_col++;
    if (TTY_col >= TTY_COLS)
    {
        TTY_col = 0;
        TTY_row++;
        if (TTY_row >= TTY_ROWS)
        {
            TTY_scroll_internal();
            TTY_row = TTY_ROWS - 1;
        }
    }
    TTY_update_hwcurs();
}

void TTY_putc(char c)
{
    switch (c)
    {
    case '\n':
        TTY_col = 0;
        TTY_row++;
        if (TTY_row >= TTY_ROWS)
        {
            TTY_scroll_internal();
            TTY_row = TTY_ROWS - 1;
        }
        TTY_update_hwcurs();
        return;
    case '\r':
        TTY_col = 0;
        TTY_update_hwcurs();
        return;
    case '\b':
        TTY_backspace();
        return;
    default:
        TTY_putch_internal(c);
        TTY_advance();
        return;
    }
}

void TTY_puts(const char *s)
{
    if (!s)
        return;

    while (*s)
        TTY_putc(*s++);
}

void TTY_backspace(void)
{
    if (TTY_col == 0)
    {
        if (TTY_row == 0)
            return;
        TTY_row--;
        TTY_col = TTY_COLS - 1;
    }
    else
    {
        TTY_col--;
    }

    if (TTY_use_fb)
    {
        uint32_t fg = tty_color_to_rgb(TTY_fg);
        uint32_t bg = tty_color_to_rgb(TTY_bg);
        fbcon_putcat(TTY_row, TTY_col, ' ', fg, bg);
        return;
    }

    volatile uint16_t *v = TTY_base;
    v[TTY_row * TTY_COLS + TTY_col] = make_entry(' ', TTY_fg, TTY_bg);
    TTY_update_hwcurs();
}

/* exposed scroll wrapper if needed */
void TTY_scroll(void)
{
    TTY_scroll_internal();
    TTY_update_hwcurs();
}
