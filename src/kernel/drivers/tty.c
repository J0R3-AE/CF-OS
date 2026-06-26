#include "drivers/tty.h"
#include "drivers/console.h"
#include <stdint.h>
#include <stddef.h>

#define VGA_MEM ((uint16_t *)0xB8000)

/* state */
static int fb_mode = 0;

static size_t row = 0, col = 0;
static size_t rows = 25, cols = 80;

static uint8_t fg = 7, bg = 0;

/* VGA entry */
static inline uint16_t vga_entry(char c, uint8_t fg, uint8_t bg)
{
    return (uint16_t)c | ((uint16_t)((bg << 4) | (fg & 0x0F)) << 8);
}

/* backend switch */
void TTY_set_fb_backend(int enable)
{
    fb_mode = enable ? 1 : 0;
}

/* init */
void TTY_init(void)
{
    if (fb_mode)
    {
        rows = console_rows();
        cols = console_cols();
    }
    else
    {
        rows = 25;
        cols = 80;
    }

    row = 0;
    col = 0;

    TTY_clear();
}

/* clear */
void TTY_clear(void)
{
    if (fb_mode)
    {
        console_clear();
        row = 0;
        col = 0;
        return;
    }

    for (size_t i = 0; i < rows * cols; i++)
        VGA_MEM[i] = vga_entry(' ', fg, bg);

    row = 0;
    col = 0;
}

/* scroll */
static void scroll(void)
{
    if (fb_mode)
    {
        console_scroll();
        if (row > 0)
            row--;
        return;
    }

    for (size_t r = 1; r < rows; r++)
    {
        for (size_t c = 0; c < cols; c++)
        {
            VGA_MEM[(r - 1) * cols + c] =
                VGA_MEM[r * cols + c];
        }
    }

    uint16_t blank = vga_entry(' ', fg, bg);

    for (size_t c = 0; c < cols; c++)
        VGA_MEM[(rows - 1) * cols + c] = blank;

    if (row > 0)
        row--;
}

/* putc */
void TTY_putc(char c)
{
    /* ---------- CONTROL CHARS ---------- */

    if (c == '\n')
    {
        col = 0;
        row++;
        goto check_scroll;
    }

    if (c == '\r')
    {
        col = 0;
        return;
    }

    if (c == '\t')
    {
        size_t spaces = 4 - (col % 4);
        while (spaces--)
            TTY_putc(' ');
        return;
    }

    if (c == '\b')
    {
        if (col > 0)
        {
            col--;

            if (fb_mode)
            {
                console_putc_at(row, col, ' ',
                                0xFFFFFF,
                                0x000000);
            }
            else
            {
                VGA_MEM[row * cols + col] =
                    vga_entry(' ', fg, bg);
            }
        }
        return;
    }

    /* ignore other control chars */
    if ((unsigned char)c < 32)
        return;

    /* ---------- PRINT ---------- */

    if (fb_mode)
    {
        console_putc_at(row, col, c,
                        0xFFFFFF,
                        0x000000);
    }
    else
    {
        VGA_MEM[row * cols + col] =
            vga_entry(c, fg, bg);
    }

    col++;

    /* wrap */
    if (col >= cols)
    {
        col = 0;
        row++;
    }

check_scroll:
    if (row >= rows)
    {
        scroll();

        if (row >= rows)
            row = rows - 1;
    }
}

/* string */
void TTY_puts(const char *s)
{
    if (!s)
        return;

    while (*s)
        TTY_putc(*s++);
}