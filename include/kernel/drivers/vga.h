#ifndef VGA_H
#define VGA_H

#include "libc/types.h"

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

void vga_text_init(void);
void vga_text_clear(u8 foreground, u8 background);
void vga_text_put_cell(usize row, usize col, char character,
                       u8 foreground, u8 background);
void vga_text_scroll(u8 foreground, u8 background);
void vga_text_set_cursor(usize row, usize col);

#endif