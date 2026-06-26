#include "drivers/vga.h"

#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static size_t g_row = 0;
static size_t g_col = 0;
static uint8_t g_color = 0x07;

static inline uint16_t vga_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

uint8_t vga_make_color(vga_color_t fg, vga_color_t bg)
{
    return (uint8_t)(fg | (bg << 4));
}

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    g_color = vga_make_color(fg, bg);
}

static void vga_scroll(void)
{
    for (size_t row = 1; row < VGA_HEIGHT; row++)
    {
        for (size_t col = 0; col < VGA_WIDTH; col++)
        {
            VGA_MEMORY[(row - 1) * VGA_WIDTH + col] =
                VGA_MEMORY[row * VGA_WIDTH + col];
        }
    }

    for (size_t col = 0; col < VGA_WIDTH; col++)
    {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            vga_entry(' ', g_color);
    }

    if (g_row > 0)
        g_row--;
}

void vga_clear(void)
{
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            VGA_MEMORY[y * VGA_WIDTH + x] = vga_entry(' ', g_color);
        }
    }

    g_row = 0;
    g_col = 0;
}

void vga_init(void)
{
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_clear();
}

void vga_set_cursor(size_t row, size_t col)
{
    if (row < VGA_HEIGHT) g_row = row;
    if (col < VGA_WIDTH) g_col = col;
}

void vga_get_cursor(size_t *row, size_t *col)
{
    if (row) *row = g_row;
    if (col) *col = g_col;
}

static void vga_put_at(char c, size_t row, size_t col)
{
    VGA_MEMORY[row * VGA_WIDTH + col] = vga_entry(c, g_color);
}

void vga_putc(char c)
{
    switch (c)
    {
    case '\n':
        g_col = 0;
        g_row++;
        if (g_row >= VGA_HEIGHT) vga_scroll();
        return;

    case '\r':
        g_col = 0;
        return;

    case '\t':
        for (int i = 0; i < 4 - (g_col % 4); i++)
            vga_putc(' ');
        return;

    case '\b':
        if (g_col > 0)
        {
            g_col--;
            vga_put_at(' ', g_row, g_col);
        }
        return;
    }

    vga_put_at(c, g_row, g_col);
    g_col++;

    if (g_col >= VGA_WIDTH)
    {
        g_col = 0;
        g_row++;
        if (g_row >= VGA_HEIGHT)
            vga_scroll();
    }
}

void vga_write_n(const char *str, size_t len)
{
    if (!str) return;
    for (size_t i = 0; i < len; i++)
        vga_putc(str[i]);
}

void vga_write(const char *str)
{
    if (!str) return;
    while (*str)
        vga_putc(*str++);
}