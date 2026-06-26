#ifndef VGA_H
#define VGA_H

#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Standard VGA 16-color palette */
typedef enum
{
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,        /* aka dark yellow in VGA */
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_LIGHT_BROWN = 14, /* bright yellow */
    VGA_WHITE = 15
} vga_color_t;

void vga_init(void);
void vga_clear(void);

void vga_set_color(vga_color_t fg, vga_color_t bg);
uint8_t vga_make_color(vga_color_t fg, vga_color_t bg);

void vga_putc(char c);
void vga_write(const char *str);
void vga_write_n(const char *str, size_t len);

void vga_set_cursor(size_t row, size_t col);
void vga_get_cursor(size_t *row, size_t *col);

#endif