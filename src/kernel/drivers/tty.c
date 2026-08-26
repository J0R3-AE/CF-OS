#include "kernel/drivers/tty.h"
#include "kernel/drivers/console.h"

#include "libc/types.h"

#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Backend/state                                                              */
/* -------------------------------------------------------------------------- */

static int fb_mode = 0;

volatile uint16_t *TTY_base = (volatile uint16_t *)VGA_MEMORY;

size_t TTY_row = 0;
size_t TTY_col = 0;

size_t TTY_ROWS = 25;
size_t TTY_COLS = 80;

uint8_t TTY_fg = TTY_COLOR_WHITE;
uint8_t TTY_bg = TTY_COLOR_BLACK;


/* -------------------------------------------------------------------------- */
/* Input                                                                       */
/* -------------------------------------------------------------------------- */

/*
 * line.c owns the actual canonical input buffer.
 * tty.c provides the terminal-facing API around it.
 */
extern int line_ready(void);
extern int line_eof(void);
extern const char *line_get_buffer(void);
extern size_t line_get_length(void);
extern void line_consume(void);


/* -------------------------------------------------------------------------- */
/* VGA                                                                         */
/* -------------------------------------------------------------------------- */

static inline uint16_t vga_entry(
    char c,
    uint8_t fg,
    uint8_t bg)
{
    return (uint16_t)c |
           ((uint16_t)(((bg & 0x0F) << 4) |
                       (fg & 0x0F)) << 8);
}


/* -------------------------------------------------------------------------- */
/* Backend                                                                     */
/* -------------------------------------------------------------------------- */

void TTY_set_fb_backend(int enable)
{
    fb_mode = enable ? 1 : 0;
}

void TTY_set_TTY_base(volatile uint16_t *addr)
{
    if (addr)
        TTY_base = addr;
}


/* -------------------------------------------------------------------------- */
/* Initialization                                                              */
/* -------------------------------------------------------------------------- */

void TTY_init(void)
{
    if (fb_mode)
    {
        TTY_ROWS = console_rows();
        TTY_COLS = console_cols();
    }
    else
    {
        TTY_ROWS = 25;
        TTY_COLS = 80;
    }

    TTY_row = 0;
    TTY_col = 0;

    TTY_resetcolor();
    TTY_clear();
}


/* -------------------------------------------------------------------------- */
/* Cursor                                                                      */
/* -------------------------------------------------------------------------- */

size_t TTY_getrow(void)
{
    return TTY_row;
}

void TTY_setrow(size_t row)
{
    if (row >= TTY_ROWS)
        row = TTY_ROWS - 1;

    TTY_row = row;
}

size_t TTY_getcol(void)
{
    return TTY_col;
}

void TTY_setcol(size_t col)
{
    if (col >= TTY_COLS)
        col = TTY_COLS - 1;

    TTY_col = col;
}

void TTY_getpos(
    size_t *row,
    size_t *col)
{
    if (row)
        *row = TTY_row;

    if (col)
        *col = TTY_col;
}

void TTY_setpos(
    size_t row,
    size_t col)
{
    TTY_setrow(row);
    TTY_setcol(col);
}

void TTY_getsize(
    size_t *rows,
    size_t *cols)
{
    if (rows)
        *rows = TTY_ROWS;

    if (cols)
        *cols = TTY_COLS;
}


/* -------------------------------------------------------------------------- */
/* Colors                                                                      */
/* -------------------------------------------------------------------------- */

void TTY_setcolor(
    uint8_t fg,
    uint8_t bg)
{
    TTY_fg = fg & 0x0F;
    TTY_bg = bg & 0x0F;
}

void TTY_getcolor(
    uint8_t *fg,
    uint8_t *bg)
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


/* -------------------------------------------------------------------------- */
/* Clear                                                                       */
/* -------------------------------------------------------------------------- */

void TTY_clear(void)
{
    if (fb_mode)
    {
        console_clear();

        TTY_row = 0;
        TTY_col = 0;

        return;
    }

    if (!TTY_base)
        return;

    uint16_t blank = vga_entry(
        ' ',
        TTY_fg,
        TTY_bg);

    for (size_t i = 0;
         i < TTY_ROWS * TTY_COLS;
         ++i)
    {
        TTY_base[i] = blank;
    }

    TTY_row = 0;
    TTY_col = 0;
}


/* -------------------------------------------------------------------------- */
/* Clear line                                                                  */
/* -------------------------------------------------------------------------- */

void TTY_clearline(size_t row)
{
    if (row >= TTY_ROWS)
        return;

    if (fb_mode)
    {
        for (size_t col = 0;
             col < TTY_COLS;
             ++col)
        {
            console_putc_at(
                row,
                col,
                ' ',
                0xFFFFFF,
                0x000000);
        }

        return;
    }

    if (!TTY_base)
        return;

    uint16_t blank = vga_entry(
        ' ',
        TTY_fg,
        TTY_bg);

    for (size_t col = 0;
         col < TTY_COLS;
         ++col)
    {
        TTY_base[row * TTY_COLS + col] = blank;
    }
}


/* -------------------------------------------------------------------------- */
/* Scroll                                                                      */
/* -------------------------------------------------------------------------- */

void TTY_scroll(void)
{
    if (fb_mode)
    {
        console_scroll();

        if (TTY_row > 0)
            TTY_row--;

        return;
    }

    if (!TTY_base)
        return;

    for (size_t row = 1;
         row < TTY_ROWS;
         ++row)
    {
        for (size_t col = 0;
             col < TTY_COLS;
             ++col)
        {
            TTY_base[(row - 1) * TTY_COLS + col] =
                TTY_base[row * TTY_COLS + col];
        }
    }

    uint16_t blank = vga_entry(
        ' ',
        TTY_fg,
        TTY_bg);

    for (size_t col = 0;
         col < TTY_COLS;
         ++col)
    {
        TTY_base[(TTY_ROWS - 1) * TTY_COLS + col] = blank;
    }

    if (TTY_row > 0)
        TTY_row--;
}


/* -------------------------------------------------------------------------- */
/* Backspace                                                                  */
/* -------------------------------------------------------------------------- */

void TTY_backspace(void)
{
    if (TTY_col == 0)
        return;

    TTY_col--;

    if (fb_mode)
    {
        console_putc_at(
            TTY_row,
            TTY_col,
            ' ',
            0xFFFFFF,
            0x000000);

        return;
    }

    if (!TTY_base)
        return;

    TTY_base[TTY_row * TTY_COLS + TTY_col] =
        vga_entry(
            ' ',
            TTY_fg,
            TTY_bg);
}


/* -------------------------------------------------------------------------- */
/* Put character                                                               */
/* -------------------------------------------------------------------------- */

void TTY_putc(char c)
{
    /* Newline */
    if (c == '\n')
    {
        TTY_col = 0;
        TTY_row++;

        if (TTY_row >= TTY_ROWS)
            TTY_scroll();

        return;
    }

    /* Carriage return */
    if (c == '\r')
    {
        TTY_col = 0;
        return;
    }

    /* Tab */
    if (c == '\t')
    {
        size_t spaces =
            4 - (TTY_col % 4);

        while (spaces--)
            TTY_putc(' ');

        return;
    }

    /* Backspace */
    if (c == '\b')
    {
        TTY_backspace();
        return;
    }

    /* Ignore remaining control characters */
    if ((uint8_t)c < 32)
        return;

    if (fb_mode)
    {
        console_putc_at(
            TTY_row,
            TTY_col,
            c,
            0xFFFFFF,
            0x000000);
    }
    else
    {
        if (!TTY_base)
            return;

        TTY_base[
            TTY_row * TTY_COLS + TTY_col
        ] = vga_entry(
            c,
            TTY_fg,
            TTY_bg);
    }

    TTY_col++;

    if (TTY_col >= TTY_COLS)
    {
        TTY_col = 0;
        TTY_row++;

        if (TTY_row >= TTY_ROWS)
            TTY_scroll();
    }
}


/* -------------------------------------------------------------------------- */
/* Put string                                                                  */
/* -------------------------------------------------------------------------- */

void TTY_puts(const char *s)
{
    if (!s)
        return;

    while (*s)
        TTY_putc(*s++);
}


/* -------------------------------------------------------------------------- */
/* Draw without moving cursor                                                 */
/* -------------------------------------------------------------------------- */

void TTY_putcat(
    size_t row,
    size_t col,
    char ch,
    uint8_t fg,
    uint8_t bg)
{
    if (row >= TTY_ROWS || col >= TTY_COLS)
        return;

    if (fb_mode)
    {
        /*
         * Framebuffer console currently takes RGB values,
         * so use a basic white/black representation here.
         * This can be replaced with a proper ANSI palette mapper.
         */
        console_putc_at(
            row,
            col,
            ch,
            fg == TTY_COLOR_BLACK ? 0x000000 : 0xFFFFFF,
            bg == TTY_COLOR_BLACK ? 0x000000 : 0xFFFFFF);

        return;
    }

    if (!TTY_base)
        return;

    TTY_base[row * TTY_COLS + col] =
        vga_entry(ch, fg, bg);
}

void TTY_putsat(
    size_t row,
    size_t col,
    const char *s,
    uint8_t fg,
    uint8_t bg)
{
    if (!s || row >= TTY_ROWS || col >= TTY_COLS)
        return;

    while (*s && col < TTY_COLS)
    {
        TTY_putcat(
            row,
            col,
            *s,
            fg,
            bg);

        s++;
        col++;
    }
}


/* -------------------------------------------------------------------------- */
/* Input                                                                       */
/* -------------------------------------------------------------------------- */

int TTY_input_ready(void)
{
    return line_ready();
}

int TTY_input_eof(void)
{
    return line_eof();
}

size_t TTY_readline(
    char *buffer,
    size_t size)
{
    if (!buffer || size == 0)
        return 0;

    if (!line_ready())
    {
        if (line_eof())
        {
            buffer[0] = '\0';
            return 0;
        }

        return 0;
    }

    const char *line = line_get_buffer();

    if (!line)
    {
        buffer[0] = '\0';
        return 0;
    }

    size_t length =
        line_get_length();

    if (length >= size)
        length = size - 1;

    for (size_t i = 0;
         i < length;
         ++i)
    {
        buffer[i] = line[i];
    }

    buffer[length] = '\0';

    return length;
}

void TTY_input_consume(void)
{
    line_consume();
}