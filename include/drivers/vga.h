/// @deprecated

#ifndef VGA_H
#define VGA_H

#include "libk/types.h"

/**
 * @file vga.h
 * @brief Low-level VGA text mode interface.
 *
 * Provides basic text output, cursor control, color management,
 * and scrolling for the 80×25 VGA text buffer at 0xB8000.
 */

/**
 * @name VGA Text Mode Dimensions
 * @{
 */
#define VGA_WIDTH 80  /**< Number of text columns. */
#define VGA_HEIGHT 25 /**< Number of text rows.    */
/** @} */

/**
 * @enum vga_color
 * @brief Standard 16‑color VGA palette.
 */
typedef enum vga_color
{
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15
} vga_color_t;

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

/// Initialize VGA text mode and clear the screen.
void vga_init(void);

/// Clear the entire VGA text buffer.
void vga_clear(void);

/// Set the current foreground and background colors.
void vga_set_color(vga_color_t fg, vga_color_t bg);

/// Write a single character at the current cursor position.
void vga_putchar(char c);

/// Write a null‑terminated string.
void vga_write(const char *str);

/// Write a string with a specified length.
void vga_write_len(const char *str, ssize len);

/// Set the hardware cursor position.
void vga_set_cursor(u8 x, u8 y);

/// Get the current hardware cursor position.
void vga_get_cursor(u8 *x, u8 *y);

/// Enable or disable the hardware cursor.
void vga_enable_cursor(u8 enable);

/// Scroll the screen up by one line.
void vga_scroll(void);

#endif /* VGA_H */
